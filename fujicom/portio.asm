; 8250 UART Serial I/O Driver for 8088
; Implements serial communication functions for COM1 (port 0x3F8)
;
; Baud Rate Configuration (1.8432 MHz crystal):
;   115200 baud: divisor = 1
;    57600 baud: divisor = 2
;    38400 baud: divisor = 3
;    19200 baud: divisor = 6
;     9600 baud: divisor = 12
;     4800 baud: divisor = 24
;     2400 baud: divisor = 48
;
; Change BAUD_DIVISOR below to set desired baud rate

	;.model	small
	;.8086

	PUBLIC	_port_uart_base

	.data

; Global variable to store UART base address
_port_uart_base	DW	3F8h		; Default to COM1

	.code

	; Baud rate divisor - change this to set baud rate
BAUD_DIVISOR	EQU	1		; 115200 baud

	; 8250 UART Register Offsets (relative to base)
UART_RBR_OFF	EQU	0		; Receiver Buffer Register (read)
UART_THR_OFF	EQU	0		; Transmitter Holding Register (write)
UART_IER_OFF	EQU	1		; Interrupt Enable Register
UART_IIR_OFF	EQU	2		; Interrupt Identification Register
UART_LCR_OFF	EQU	3		; Line Control Register
UART_MCR_OFF	EQU	4		; Modem Control Register
UART_LSR_OFF	EQU	5		; Line Status Register
UART_MSR_OFF	EQU	6		; Modem Status Register
UART_DLL_OFF	EQU	0		; Divisor Latch Low (when DLAB=1)
UART_DLH_OFF	EQU	1		; Divisor Latch High (when DLAB=1)

	; Line Status Register bits
LSR_DR		EQU	01h		; Data Ready
LSR_THRE	EQU	20h		; Transmitter Holding Register Empty

	; Line Control Register bits
LCR_DLAB	EQU	80h		; Divisor Latch Access Bit
LCR_8N1		EQU	03h		; 8 data bits, no parity, 1 stop bit

	; Modem Control Register bits
MCR_DTR		EQU	01h		; Data Terminal Ready
MCR_RTS		EQU	02h		; Request To Send
MCR_OUT2	EQU	08h		; OUT2 (enables interrupts on PC)

	; BIOS Data Area
BIOS_DATA_SEG	EQU	40h
BIOS_TICK_OFFSET EQU	6Ch

	; SLIP Protocol Constants
SLIP_END	EQU	0C0h
SLIP_ESC	EQU	0DBh
SLIP_ESC_END	EQU	0DCh
SLIP_ESC_ESC	EQU	0DDh

	.code

; Debug helper - write character to QEMU debug port 0xE9
qemu_debug_char PROC	NEAR
	push	dx
	push	ax
	mov	dx, 0E9h
	out	dx, al
	pop	ax
	pop	dx
	ret
qemu_debug_char ENDP

	include port_init.asm
	include port_getbuf_slip_dual.asm
	include port_putc.asm
	include port_putbuf_slip.asm

	END
