/**
 * #FUJINET Low Level Routines
 */

#undef DEBUG
#define INIT_INFO

#include "fujicom.h"
#include "portio.h"
#include <dos.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#if defined(DEBUG) || defined(INIT_INFO)
#include "../sys/print.h" // debug
#endif

#include <env.h>

#define TIMEOUT         100
#define TIMEOUT_SLOW	15 * 1000
#define MAX_RETRIES	1
#ifndef SERIAL_BPS
#define SERIAL_BPS      115200
#endif /* SERIAL_BPS */

union REGS f5regs;
struct SREGS f5status;

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
  uint8_t far *data;
} fujibus_packet;

#define MAX_PACKET (sizeof(fujibus_header) + 4) // header + aux
static uint8_t fb_buffer[MAX_PACKET];
static fujibus_packet *fb_packet = (fujibus_packet *) fb_buffer;

// Not worth making these into functions, I'm sure they'd eat more bytes
const uint8_t fuji_field_numbytes_table[] = {0, 1, 2, 3, 4, 2, 4, 4};
#define fuji_field_numbytes(descr) fuji_field_numbytes_table[descr]

void fujicom_init(void)
{
  unsigned divisor;
  const char *fuji_port, *comma;
  unsigned port_len;
  unsigned long bps = SERIAL_BPS;
  int comp = 1;
  unsigned base = COM1_UART, irq = COM1_INTERRUPT;


  if (getenv("FUJI_BPS"))
    bps = strtoul(getenv("FUJI_BPS"), NULL, 10);

  fuji_port = getenv("FUJI_PORT");
  if (fuji_port) {
    comma = strchr(fuji_port, ',');
    if (comma)
      port_len = comma - fuji_port;
    else
      port_len = strlen(fuji_port);

    if (!strncasecmp(fuji_port, "0x", 2))
      base = strtoul(fuji_port + 2, NULL, 16);
    else if (tolower(fuji_port[port_len - 1]) == 'h')
      base = strtoul(fuji_port, NULL, 16);
    else {
      comp = atoi(fuji_port);
      switch (comp) {
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
    }

    if (comma)
      irq = atoi(comma + 1);
  }

  divisor = 115200UL / bps;
  port_init(base, divisor);
#if defined(DEBUG) || defined(INIT_INFO)
  consolef("Port: %xh  BPS: %ld/%d\n", port_uart_base, (int32_t) bps, divisor);
#endif

  return;
}

uint16_t fuji_calc_checksum(const void far *ptr, uint16_t len, uint16_t seed)
{
  uint16_t idx, chk;
  uint8_t far *buf = (uint8_t far *) ptr;


  for (idx = 0, chk = seed; idx < len; idx++)
    chk = ((chk + buf[idx]) >> 8) + ((chk + buf[idx]) & 0xFF);
  return chk;
}

void packet_fail(fujibus_packet *packet, uint16_t rlen, const char *message, ...)
{
  va_list args;
  uint16_t hlen = sizeof(packet->header);


#ifdef MEGA_DEBUG
  dumpHex(packet, hlen, 0);
  if (rlen > hlen)
    dumpHex(packet->data, rlen - hlen, hlen);
#endif // MEGA_DEBUG
  va_start(args, message);
  vconsolef(message, args);
  va_end(args);
  return;
}

bool fuji_bus_call(uint8_t device, uint8_t fuji_cmd, uint8_t fields,
		   uint8_t aux1, uint8_t aux2, uint8_t aux3, uint8_t aux4,
		   const void far *data, size_t data_length,
		   void far *reply, size_t reply_length)
{
  int code;
  uint16_t ck1, ck2;
  uint16_t rlen;
  uint16_t idx, numbytes;
  uint8_t *ptr = &fb_buffer[sizeof(fujibus_header)];


  fb_packet->header.device = device;
  fb_packet->header.command = fuji_cmd;
  fb_packet->header.length = sizeof(fujibus_header);
  fb_packet->header.checksum = 0;
  fb_packet->header.fields = fields;
  fb_packet->data = ptr;

  idx = 0;
  numbytes = fuji_field_numbytes(fields);
  if (numbytes) {
    ptr[idx++] = aux1;
    numbytes--;
  }
  if (numbytes) {
    ptr[idx++] = aux2;
    numbytes--;
  }
  if (numbytes) {
    ptr[idx++] = aux3;
    numbytes--;
  }
  if (numbytes) {
    ptr[idx++] = aux4;
    numbytes--;
  }

  fb_packet->header.length += idx + data_length;

  // Data is spread across two buffers: ours and data
  ck1 = fuji_calc_checksum(fb_packet, sizeof(fb_packet->header) + idx, 0);
  if (data)
    ck1 = fuji_calc_checksum(data, data_length, ck1);
  fb_packet->header.checksum = ck1;

  port_putc(SLIP_END);
  port_putbuf_slip(fb_buffer, idx + sizeof(fb_packet->header));
  if (data)
    port_putbuf_slip(data, data_length);
  port_putc(SLIP_END);

  fb_packet->data = reply;
  rlen = port_getbuf_slip_dual(fb_packet, sizeof(fb_packet->header),
                               fb_packet->data, reply_length,
                               TIMEOUT_SLOW);

#if 0 //def DEBUG
  if (rlen)
    dumpHex(fb_packet, rlen, 0);
  consolef("RECEIVED LEN %d\n", rlen);
#endif
  if (rlen < sizeof(fujibus_header) || rlen != fb_packet->header.length) {
#ifdef DEBUG
    packet_fail(fb_packet, rlen,
                "SHORT PACKET R:%d E:%d\n", rlen, fb_packet->header.length);
#endif
    return false;
  }

  // FIXME - validate that fb_packet->fields is zero?

  // Need to zero out checksum in order to calculate
  ck1 = fb_packet->header.checksum;
  fb_packet->header.checksum = 0;

  // Data is spread across two buffers: ours and reply
  ck2 = fuji_calc_checksum(fb_packet, sizeof(fb_packet->header), 0);
  if (fb_packet->data)
    ck2 = fuji_calc_checksum(fb_packet->data, rlen - sizeof(fujibus_header), ck2);
  ck2 = (uint8_t) ck2;

  if (ck1 != ck2) {
#ifdef DEBUG
    packet_fail(fb_packet, rlen, "CHECKSUM MISMATCH C:%02x E:%02x\n", ck2, ck1);
#endif
    return false;
  }

  if (fb_packet->header.device != device) {
#ifdef DEBUG
    packet_fail(fb_packet, rlen,
                "WRONG DEVICE %02x != %02x\n", fb_packet->header.device, device);
#endif
    return false;
  }

  if (fb_packet->header.command != PACKET_ACK) {
#ifdef DEBUG
    packet_fail(fb_packet, rlen, "NOT ACK 0x%02x\n", fb_packet->header.command);
#endif
    return false;
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
