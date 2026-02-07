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

	.code

	PUBLIC	_port_init
	PUBLIC	_port_getc
	PUBLIC	_port_getc_timeout
	PUBLIC	_port_getbuf
	PUBLIC	_port_getbuf_sentinel
	PUBLIC	_port_putc
	PUBLIC	_port_putbuf
	PUBLIC	_port_uart_base

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
	include port_getc.asm
	include port_getc_timeout.asm
	include port_getbuf.asm
	include port_getbuf_sentinel.asm
	include port_putc.asm
	include port_putbuf.asm

	END
