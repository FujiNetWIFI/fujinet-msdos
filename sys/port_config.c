/**
 * FujiNet serial port configuration.
 */

#undef DEBUG
#define INIT_INFO

#include "fujicom.h"
#include "portio.h"
#include <ctype.h>
#include <dos.h>
#include <string.h>
#include <strings.h>

#if defined(DEBUG) || defined(INIT_INFO)
#include "../sys/print.h"
#endif

#include <env.h>

#ifndef SERIAL_BPS
#define SERIAL_BPS      115200
#endif /* SERIAL_BPS */

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

void fujicom_done(void)
{
  return;
}
