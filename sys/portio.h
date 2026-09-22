#include <stdint.h>
#include "machine.h"

#if MACH_HAS_8250
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
#endif /* MACH_HAS_8250 */

/*
 * port_init's "base" is whatever the machine's backend needs to find its
 * registers: an I/O address on most machines, a register segment on the
 * memory-mapped ones.  FUJI_PORT's hex form sets it directly; the COM1-4
 * numeric form only means anything on a PC.
 */

extern uint16_t port_uart_base;

extern void cdecl port_init(uint16_t base, uint16_t divisor);
extern uint16_t cdecl port_getbuf_slip_dual(void *hdr_buf, uint16_t hdr_len,
                                            void far *data_buf, uint16_t data_len,
                                            uint16_t timeout);
extern int cdecl port_putc(uint8_t c);
extern uint16_t cdecl port_putbuf_slip(const void far *buf, uint16_t len);
