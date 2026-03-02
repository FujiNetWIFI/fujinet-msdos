#include "id8250.h"
#include "portio.h"

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

#ifdef M_I86
#include <conio.h>
#define inportb inp
#define outportb outp
#endif /* M_I86 */

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
