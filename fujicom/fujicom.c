/**
 * #FUJINET Low Level Routines
 */

#define DEBUG
#define INIT_INFO

#include "fujicom.h"
#include "portio.h"
#include <dos.h>
#include <string.h>

#if defined(DEBUG) || defined(INIT_INFO)
#include "../sys/print.h" // debug
#endif

#include <env.h>

#define TIMEOUT         100
#define TIMEOUT_SLOW	15 * 1000
#define MAX_RETRIES	5
#ifndef SERIAL_BPS
#define SERIAL_BPS      115200
#endif /* SERIAL_BPS */

#ifdef M_I86
#include <conio.h>
#define inportb inp
#define outportb outp
#endif /* M_I86 */

/*
 * This group of defines creates all the definitions used
 * to access registers and bit fields in the 8250 UART.
 * While the names may not be the best, they are the ones
 * used in the 8250 data sheets, so they are generally not
 * changed.  Since the definitions are only used in COM.C,
 * they are not included in the COM.H header file where
 * they might normally be expected.
 */
#define RBR              0      /* Receive buffer register */
#define THR              0      /* Transmit holding reg.   */
#define IER              1      /* Interrupt Enable reg.   */
#define IER_RX_DATA      1      /* Enable RX interrupt bit */
#define IER_THRE         2      /* Enable TX interrupt bit */
#define IIR              2      /* Interrupt ID register   */
#define IIR_MODEM_STATUS 0      /* Modem stat. interrupt ID */
#define IIR_TRANSMIT     2      /* Transmit interrupt ID   */
#define IIR_RECEIVE      4      /* Receive interrupt ID    */
#define IIR_LINE_STATUS  6      /* Line stat. interrupt ID */
#define LCR              3      /* Line control register   */
#define LCR_DLAB         0x80   /* Divisor access bit      */
#define LCR_EVEN_PARITY  0x8    /* Set parity 'E' bits     */
#define LCR_ODD_PARITY   0x18   /* Set parity 'O' bits     */
#define LCR_NO_PARITY    0      /* Set parity 'N' bits     */
#define LCR_1_STOP_BIT   0      /* Bits to set 1 stop bit  */
#define LCR_2_STOP_BITS  4      /* Bits to set 2 stop bits */
#define LCR_5_DATA_BITS  0      /* Bits to set 5 data bits */
#define LCR_6_DATA_BITS  1      /* Bits to set 6 data bits */
#define LCR_7_DATA_BITS  2      /* Bits to set 7 data bits */
#define LCR_8_DATA_BITS  3      /* Bits to set 8 data bits */
#define MCR              4      /* Modem control register  */
#define MCR_DTR          1      /* Bit to turn on DTR      */
#define MCR_RTS          2      /* Bit to turn on RTS      */
#define MCR_OUT1         4      /* Bit to turn on OUT1     */
#define MCR_OUT2         8      /* Bit to turn on OUT2     */
#define MCR_LOOP         16     /* Bit to turn on loopback */
#define LSR              5      /* Line Status register    */
#define MSR              6      /* Modem Status register   */
#define DLL              0      /* Divisor latch LSB       */
#define DLM              1      /* Divisor latch MSB       */
#define SCR              7      /* Scratch register        */
#define FCR              2      /* FIFO Control Register   */

union REGS f5regs;
struct SREGS f5status;

void fujicom_init(void)
{
  int base, irq;
  int divisor;
  long bps = SERIAL_BPS;
  int comp = 1;


  // FIXME - allow FUJI_PORT to be "<base>,<irq>" in addition to 1-4
  if (getenv("FUJI_PORT"))
    comp = atoi(getenv("FUJI_PORT"));
  if (getenv("FUJI_BPS"))
    bps = atol(getenv("FUJI_BPS"));

  switch (comp) {
  default:
  case 1:
    base = COM1_UART;
    irq = COM1_INTERRUPT;
    break;
  case 2:
    base = COM2_UART;
    irq = COM2_INTERRUPT;
    break;
  case 3:
    base = COM3_UART;
    irq = COM3_INTERRUPT;
    break;
  case 4:
    base = COM4_UART;
    irq = COM4_INTERRUPT;
    break;
  }

  divisor = 115200L / bps;
#if defined(DEBUG) || defined(INIT_INFO)
  consolef("Port: %xH  BPS: %ld/%d\n", base, (int32_t) bps, divisor);
#endif

  port_init(base, divisor);

  return;
}

enum {
  SLIP_END     = 0xC0,
  SLIP_ESCAPE  = 0xDB,
  SLIP_ESC_END = 0xDC,
  SLIP_ESC_ESC = 0xDD,
};

enum {
  PACKET_ACK = 6, // ASCII ACK
  PACKET_NAK = 21, // ASCII NAK
};

typedef struct {
  uint8_t device;   /* Destination Device */
  uint8_t command;  /* Command */
  uint16_t length;  /* Total length of packet including header */
  uint8_t checksum; /* Checksum of entire packet */
  uint8_t fields;   /* Describes the fields that follow */
} fujibus_header;

typedef struct {
  fujibus_header header;
  uint8_t data[];
} fujibus_packet;

#define MAX_PACKET      (512 + sizeof(fujibus_header) + 4) // sector + header + secnum
static uint8_t fb_buffer[MAX_PACKET * 2 + 2];              // Enough room for SLIP encoding
static fujibus_packet *fb_packet;

// Not worth making these into functions, I'm sure they'd eat more bytes
const uint8_t fuji_field_numbytes_table[] = {0, 1, 2, 3, 4, 2, 4, 4};
#define fuji_field_numbytes(descr) fuji_field_numbytes_table[descr]

#if 0
int port_discard_until(uint8_t c, uint16_t timeout)
{
  int code;


  while (1) {
    code = port_getc_timeout(TIMEOUT_SLOW);
    if (code < 0 || code == SLIP_END)
      break;
  }

  return code;
}

uint16_t port_get_until(void *buf, uint16_t maxlen, uint8_t endc, uint16_t timeout)
{
  int c;
  uint16_t idx;
  uint8_t *ptr = (uint8_t *) buf;


  for (idx = 0; idx < maxlen; idx++) {
    c = port_getc_timeout(timeout);
    if (c < 0)
      break;
    ptr[idx] = c;
    if (c == endc)
      break;
  }

  return idx;
}
#endif

/* This function expects that fb_packet is one byte into fb_buffer so
   that there's already room at the front for the SLIP_END framing
   byte. This allows skipping moving all the bytes if no escaping is
   needed. */
uint16_t fuji_slip_encode()
{
  uint16_t idx, len, enc_idx, esc_count, esc_remain;
  uint8_t ch, *ptr;


  // Count how many bytes need to be escaped
  len = fb_packet->header.length;
  ptr = (uint8_t *) fb_packet;
  for (idx = esc_count = 0; idx < len; idx++) {
    if (ptr[idx] == SLIP_END || ptr[idx] == SLIP_ESCAPE)
      esc_count++;
  }

  if (esc_count) {
    // Encode buffer in place working from back to front
    for (esc_remain = esc_count, enc_idx = esc_count + (idx = len - 1);
         esc_remain;
         idx--, enc_idx--) {
      ch = ptr[idx];
      if (ch == SLIP_END) {
        ptr[enc_idx--] = SLIP_ESC_END;
        ch = SLIP_ESCAPE;
        esc_remain--;
      }
      else if (ch == SLIP_ESCAPE) {
        ptr[enc_idx--] = SLIP_ESC_ESC;
        ch = SLIP_ESCAPE;
        esc_remain--;
      }

      ptr[enc_idx] = ch;
    }
  }

  // FIXME - this byte probably never changes, maybe we should init fb_buffer with it?
  fb_buffer[0] = SLIP_END;
  fb_buffer[1 + len + esc_count] = SLIP_END;
  return 2 + len + esc_count;
}

uint16_t fuji_slip_decode(uint16_t len)
{
  uint16_t idx, dec_idx, esc_count;
  uint8_t *ptr;


  ptr = (uint8_t *) fb_packet;
  for (idx = 0; idx < len; idx++) {
    if (ptr[idx] == SLIP_END)
      break;
  }
  idx++;

  for (dec_idx = 0; idx < len; idx++, dec_idx++) {
    if (ptr[idx] == SLIP_END)
      break;

    if (ptr[idx] == SLIP_ESCAPE) {
      idx++;
      if (ptr[idx] == SLIP_ESC_END)
        ptr[dec_idx] = SLIP_END;
      else if (ptr[idx] == SLIP_ESC_ESC)
        ptr[dec_idx] = SLIP_ESCAPE;
    }
    else if (idx != dec_idx) {
      // Only need to move byte if there were escapes decoded
      ptr[dec_idx] = ptr[idx];
    }
  }

  return dec_idx;
}

uint8_t fuji_calc_checksum(void *ptr, uint16_t len)
{
  uint16_t idx, chk;
  uint8_t *buf = (uint8_t *) ptr;


  for (idx = chk = 0; idx < len; idx++)
    chk = ((chk + buf[idx]) >> 8) + ((chk + buf[idx]) & 0xFF);
  return (uint8_t) chk;
}

bool fuji_bus_call(uint8_t device, uint8_t fuji_cmd, uint8_t fields,
		   uint8_t aux1, uint8_t aux2, uint8_t aux3, uint8_t aux4,
		   const void far *data, size_t data_length,
		   void far *reply, size_t reply_length)
{
  int code;
  uint8_t ck1, ck2;
  uint16_t rlen;
  uint16_t idx, numbytes;


  fb_packet = (fujibus_packet *) (fb_buffer + 1); // +1 for SLIP_END
  fb_packet->header.device = device;
  fb_packet->header.command = fuji_cmd;
  fb_packet->header.length = sizeof(fujibus_header);
  fb_packet->header.checksum = 0;
  fb_packet->header.fields = fields;

  idx = 0;
  numbytes = fuji_field_numbytes(fields);
  if (numbytes) {
    fb_packet->data[idx++] = aux1;
    numbytes--;
  }
  if (numbytes) {
    fb_packet->data[idx++] = aux2;
    numbytes--;
  }
  if (numbytes) {
    fb_packet->data[idx++] = aux3;
    numbytes--;
  }
  if (numbytes) {
    fb_packet->data[idx++] = aux4;
    numbytes--;
  }
  if (data) {
    _fmemcpy(&fb_packet->data[idx], data, data_length);
    idx += data_length;
  }

  fb_packet->header.length += idx;

  ck1 = fuji_calc_checksum(fb_packet, fb_packet->header.length);
  fb_packet->header.checksum = ck1;

  numbytes = fuji_slip_encode();
  port_putbuf(fb_buffer, numbytes);

  rlen = port_getbuf_sentinel(fb_packet, sizeof(fb_buffer), TIMEOUT_SLOW, SLIP_END, 2);

#if 0 //def DEBUG
  consolef("RECEIVED LEN %d\n", rlen);
  if (rlen)
    dumpHex(fb_packet, rlen, 0);
#endif
  numbytes = fuji_slip_decode(rlen);
  if (numbytes < sizeof(fujibus_header) || numbytes != fb_packet->header.length) {
#ifdef DEBUG
    consolef("SHORT PACKET R:%d N:%d E:%d\n", rlen, numbytes, fb_packet->header.length);
#endif
    return false;
  }

  // Need to zero out checksum in order to calculate
  ck1 = fb_packet->header.checksum;
  fb_packet->header.checksum = 0;
  ck2 = fuji_calc_checksum(fb_packet, numbytes);
  if (ck1 != ck2) {
#ifdef DEBUG
    consolef("CHECKSUM MISMATCH C:%02x E:%02x\n", ck1, ck2);
#endif
    return false;
  }

  // FIXME - validate that fb_packet->fields is zero?

  if (reply_length && numbytes) {
    if (reply_length < numbytes)
      numbytes = reply_length;
    _fmemcpy(reply, fb_packet->data, numbytes);
  }

  return true;
}

void fujicom_done(void)
{
  return;
}

#ifdef FUJIF5_AS_FUNCTION
int fujiF5(uint8_t direction, uint8_t device, uint8_t command, uint8_t descr,
	   uint16_t aux12, uint16_t aux34, void far *buffer, uint16_t length)
{
  int result;
  f5regs.x.dl = direction;
  f5regs.x.dh = descr;
  f5regs.h.al = device;
  f5regs.h.ah = command;
  f5regs.x.cx = aux12;
  f5regs.x.si = aux34;

  f5status.es  = FP_SEG(buffer);
  f5regs.x.bx = FP_OFF(buffer);
  f5regs.x.di = length;

  int86x(FUJINET_INT, &f5regs, &f5regs, &f5status);
  return f5regs.x.ax;
}
#endif

// FIXME - doesn't belong in this file
int port_identify_uart()
{
  int val;

  /* Check if UART has scratch register, 16450 and up do */
  outportb(port_uart_base + 7, 0x55);
  val = inportb(port_uart_base + SCR);
  if (val != 0x55)
    return UART_8250;
  outportb(port_uart_base + 7, 0xAA);
  val = inportb(port_uart_base + SCR);
  if (val != 0xAA)
    return UART_8250;

  /* Check if FIFO can be enabled, doesn't work on 8250/16450 */
  outportb(port_uart_base + FCR, 0x01);
  val = inportb(port_uart_base + IIR);
  val >>= 6; // Isolate FIFO status bits
  if (!val)
    return UART_16450;

  if (val != 3) {
    /* FIFOs don't work on UART_16550, turn it off */
    outportb(port_uart_base + FCR, 0x00);
    return UART_16550;
  }

  return UART_16550A;
}
