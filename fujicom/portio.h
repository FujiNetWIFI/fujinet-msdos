#include <stdint.h>

/*
 * These are the standard UART addresses and interrupt
 * numbers for the four IBM compatible COM ports.
 */
#define COM1_UART         0x3f8
#define COM1_INTERRUPT    12
#define COM2_UART         0x2f8
#define COM2_INTERRUPT    11
#define COM3_UART         0x3e8
#define COM3_INTERRUPT    12
#define COM4_UART         0x2e8
#define COM4_INTERRUPT    11

extern uint16_t port_uart_base;

extern void cdecl port_init(uint16_t base, uint16_t divisor);

extern int cdecl port_getc();
extern int cdecl port_getc_timeout(uint16_t timeout);
extern uint16_t cdecl port_getbuf(void *buf, uint16_t len, uint16_t timeout);
extern uint16_t cdecl port_getbuf_sentinel(void *buf, uint16_t len, uint16_t timeout,
                                           uint8_t sentinel, uint16_t sentinel_count);


extern int cdecl port_putc(uint8_t c);
extern uint16_t cdecl port_putbuf(const void *buf, uint16_t len);

#define PORT_TICKS_PER_SECOND 1000
