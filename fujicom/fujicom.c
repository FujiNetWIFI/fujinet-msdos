/**
 * #FUJINET Low Level Routines
 */

#undef DEBUG
#define INIT_INFO

#include "fujicom.h"
#include "com.h"
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

PORT fn_port;
PORT far *port;
union REGS f5regs;
struct SREGS f5status;

void fujicom_init(void)
{
  int base, irq;
  long bps = SERIAL_BPS;
  int comp = 1;


  if (getenv("FUJI_PORT"))
    comp = atoi(getenv("FUJI_PORT"));
  if (getenv("FUJI_BPS"))
    bps = atol(getenv("FUJI_BPS"));

#if defined(DEBUG) || defined(INIT_INFO)
  consolef("Port: %i  BPS: %li\n", comp, (int32_t) bps);
#endif

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

  port = port_open(&fn_port, base, irq);
  port_set(port, bps, 'N', 8, 1);
  port_disable_interrupts(port);

  return;
}

#ifdef PRE_FEP004
uint8_t fujicom_cksum(void far *ptr, uint16_t len)
{
  uint16_t chk = 0;
  int i = 0;
  uint8_t far *buf = (uint8_t far *) ptr;


  for (i = 0; i < len; i++)
    chk = ((chk + buf[i]) >> 8) + ((chk + buf[i]) & 0xFF);
  return (uint8_t) chk;
}


/**
 * @brief Internal function, send command, get response.
 *
 * @param c ptr to command frame to send
 * @return 'A'ck, or 'N'ak.
 */
int _fujicom_send_command(cmdFrame_t far *cmd)
{
  uint8_t *cc = (uint8_t *) cmd;


  /* Calculate checksum and place in frame */
  cmd->cksum = fujicom_cksum(cc, sizeof(cmdFrame_t) - sizeof(cmd->cksum));

  /* Assert DTR to indicate start of command frame */
  port_set_dtr(port, 1);

  /* Write command frame */
  port_putbuf(port, cc, sizeof(cmdFrame_t));

  port_wait_for_tx_empty(port);
  /* Desert DTR to indicate end of command frame */
  port_set_dtr(port, 0);
  return port_getc_nobuf(port, TIMEOUT);
}

int fujicom_command(cmdFrame_t far *cmd)
{
  int reply;


  //port_disable_interrupts(port);
  _fujicom_send_command(cmd);
  reply = port_getc_nobuf(port, TIMEOUT);
  //port_enable_interrupts(port);
#ifdef DEBUG
  consolef("FN command reply: 0x%04x\n", reply);
#endif

  return reply;
}

int fujicom_command_read(cmdFrame_t far *cmd, void far *ptr, uint16_t len)
{
  int reply;
  uint16_t rlen;
  int retries;
  uint8_t ck1, ck2;

  //port_disable_interrupts(port);

  for (retries = 0; retries < MAX_RETRIES; retries++) {
#ifdef DEBUG
    if (retries)
      consolef("FN read retry: %i\n", retries);
#endif

    // Flush any bytes left in the buffer
    port_wait_for_rx_empty(port);

    reply = _fujicom_send_command(cmd);
    if (reply == 'N')
      break;

    if (reply != 'A') {
#ifdef DEBUG
      consolef("FN send command bad: 0x%04x\n", reply);
#endif
      continue;
    }

    break;
  }

  if (retries == MAX_RETRIES)
    goto done;

  /* Get COMPLETE/ERROR */
  reply = port_getc_nobuf(port, TIMEOUT_SLOW);
  if (reply != 'C') {
#ifdef DEBUG
    consolef("FN complete fail: 0x%04x\n", reply);
#endif
    goto done;
  }

  /* Complete, get payload */
  rlen = port_getbuf(port, ptr, len, TIMEOUT);
  if (rlen != len) {
#ifdef DEBUG
    consolef("FN Read failed: Exp:%i Got:%i  Cmd: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
	     len, rlen,
	     cmd->device, cmd->comnd, cmd->aux1, cmd->aux2, cmd->cksum);
#endif
    reply = 'E';
    goto done;
  }

  /* Get Checksum byte, verify it. */
  ck1 = port_getc_nobuf(port, TIMEOUT_SLOW);
  ck2 = fujicom_cksum(ptr, len);

  if (ck1 != ck2) {
#ifdef DEBUG
    consolef("FN read checksum error: 0x%02x 0x%02x\n", ck1, ck2);
#endif
    reply = 'E';
  }

 done:
  //port_enable_interrupts(port);
#ifdef DEBUG
  consolef("FN command read reply: 0x%04x\n", reply);
#endif
  return reply;
}

int fujicom_command_write(cmdFrame_t far *cmd, void far *ptr, uint16_t len)
{
  int reply;
  uint8_t ck;
  int retries;

  //port_disable_interrupts(port);

  for (retries = 0; retries < MAX_RETRIES; retries++) {
#ifdef DEBUG
    if (retries)
      consolef("FN write retry: %i\n", retries);
#endif

    // Flush any bytes left in the buffer
    port_wait_for_rx_empty(port);

    reply = _fujicom_send_command(cmd);
    if (reply == 'N')
      break;

    if (reply != 'A') {
#ifdef DEBUG
      consolef("FN write command bad: 0x%04x\n", reply);
#endif
      continue;
    }

    break;
  }

  if (retries == MAX_RETRIES)
    goto done;

  /* Write the payload */
  port_putbuf(port, ptr, len);

  /* Write the checksum */
  ck = fujicom_cksum(ptr, len);
  port_putc_nobuf(port, ck);

  /* Wait for ACK/NACK */
  reply = port_getc_nobuf(port, TIMEOUT_SLOW);
  if (reply != 'A') {
#ifdef DEBUG
    consolef("FN write ack fail: 0x%04x\n", reply);
#endif
    goto done;
  }

  /* Wait for COMPLETE/ERROR */
  reply = port_getc_nobuf(port, TIMEOUT_SLOW);
  if (reply != 'C') {
#ifdef DEBUG
    consolef("FN write complete fail: 0x%04x\n", reply);
#endif
  }

 done:
  //port_enable_interrupts(port)
#ifdef DEBUG
  consolef("FN command write reply: 0x%04x\n", reply);
#endif
  return reply;
}
#else /* ! PRE_FEP004 */
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

int port_discard_until(PORT far *port, uint8_t c, uint16_t timeout)
{
  int code;


  while (1) {
    code = port_getc_nobuf(port, TIMEOUT_SLOW);
    if (code < 0 || code == SLIP_END)
      break;
  }

  return code;
}

uint16_t port_get_until(PORT far *port, void *buf, uint16_t maxlen, uint8_t endc,
                        uint16_t timeout)
{
  int c;
  uint16_t idx;
  uint8_t *ptr = (uint8_t *) buf;


  for (idx = 0; idx < maxlen; idx++) {
    c = port_getc_nobuf(port, timeout);
    if (c < 0)
      break;
    ptr[idx] = c;
    if (c == endc)
      break;
  }

  return idx;
}

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
  for (idx = dec_idx = 0; idx < len; idx++, dec_idx++) {
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

  port_putbuf(port, fb_buffer, numbytes);
  code = port_discard_until(port, SLIP_END, TIMEOUT_SLOW);
  if (code != SLIP_END)
    return false;

  rlen = port_get_until(port, fb_packet,
                        (fb_buffer + sizeof(fb_buffer)) - ((uint8_t *) fb_packet),
                        SLIP_END, TIMEOUT_SLOW);
  rlen = fuji_slip_decode(rlen);
  if (rlen != fb_packet->header.length)
    return false;

  // Need to zero out checksum in order to calculate
  ck1 = fb_packet->header.checksum;
  fb_packet->header.checksum = 0;
  ck2 = fuji_calc_checksum(fb_packet, rlen);
  if (ck1 != ck2)
    return false;

  // FIXME - validate that fb_packet->fields is zero?

  if (reply_length && rlen) {
    if (reply_length < rlen)
      rlen = reply_length;
    _fmemcpy(reply, fb_packet->data, rlen);
  }

  return true;
}
#endif /* PRE_FEP004 */

void fujicom_done(void)
{
  port_close(port);
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
