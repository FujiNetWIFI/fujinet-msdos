;-----------------------------------------------------------------------------
; IBM PC and compatibles - 8250/16450/16550 UART backend.
;
; This is the reference backend and the default one.  It documents the
; contract every other backend under mach/ has to satisfy, so read this
; first before adding a machine.
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
; The divisor is computed on the C side (fujicom.c) and passed to port_init.
;
;
; THE BACKEND CONTRACT
; ====================
;
; A backend must provide the data word _port_uart_base, the procedure
; _port_init, the constant PORT_POLL_BURST, and these macros:
;
;   PORT_HW_SETUP           Once per routine, before any register access.
;                           Sets up whatever addressing the chip needs.
;                           Always expanded while DS still addresses the
;                           driver's own data, so it may read
;                           _port_uart_base; the SLIP routines switch DS to
;                           the caller's buffer only afterwards.
;                           Empty here: the 8250 lives in I/O space and each
;                           access loads DX itself.  A memory-mapped backend
;                           points ES at its register segment here.
;
;   PORT_TIME_SETUP         Once per receive, before any PORT_TIME_* use.
;                           Sets up whatever the timebase needs.  Points ES
;                           at the BIOS data area here.  Empty on backends
;                           that count loop iterations instead.
;
;   PORT_RX_STATUS_ADDR b   Prepare for repeated PORT_RX_TEST.  Hoisted out
;                           of the poll loop, so put anything invariant here.
;                           'b' is where the 16-bit port base can be read
;                           from - it is NOT always _port_uart_base, because
;                           the SLIP routines switch DS and have to cache the
;                           base on the stack.
;
;   PORT_RX_TEST ready      One non-blocking poll.  Jump to 'ready' if a byte
;                           is waiting, else fall through.  Runs in the
;                           innermost loop, so keep it short.
;                           May clobber AL and DX.  Must preserve AH, which
;                           holds the poll-burst counter.
;
;   PORT_RX_DATA b          AL = the received byte.
;
;   PORT_TX_WAIT b          Spin until the transmitter will accept a byte.
;                           May clobber AL and DX; callers that already hold
;                           a byte in AL save it around this.
;
;   PORT_TX_DATA b          Transmit AL.
;
;   PORT_TIME_MARK t        Start (or restart) the per-byte timeout.  't' is
;                           where the timeout value can be read from.
;                           May clobber AX: the decoder only marks at points
;                           where the last received byte is already spent.
;
;   PORT_TIME_CHECK again,t Re-enable interrupts, then jump to 'again' if the
;                           timeout has not expired yet; fall through if it
;                           has.  May clobber AX.  Must preserve SI, which
;                           PORT_TIME_MARK owns, and BX/CX/DI/DS, which the
;                           SLIP decoder is using.
;
; A backend that needs private state beyond SI gets stack words from the
; decoder: PORT_RX_FRAME_PUSH is expanded after the last of the decoder's
; own pushes, so the first backend word is at [bp-18], the second [bp-20],
; and so on.  PORT_RX_FRAME_POP must pop exactly as many as it pushed.  The
; PC needs none, which is what keeps its stack frame unchanged.
;
; ES is the one register the backend may claim, and only for ONE purpose:
; either register addressing (PORT_HW_SETUP) or the timebase
; (PORT_TIME_SETUP), never both.  Declare which by defining
; PORT_NEEDS_ES_FOR_HW or PORT_NEEDS_ES_FOR_TIME; portio.asm rejects a
; backend that asks for both.
;-----------------------------------------------------------------------------

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

	; BIOS Data Area
BIOS_DATA_SEG	EQU	40h
BIOS_TICK_OFFSET EQU	6Ch

	; Poll the line status this many times between timeout checks.  On an
	; 8088 this amortises the tick read without overshooting a byte time.
PORT_POLL_BURST	EQU	32

	; The BIOS tick counter is the timebase, so ES belongs to it.
PORT_NEEDS_ES_FOR_TIME EQU 1

;-----------------------------------------------------------------------------
; Register access.  The 8250 is in I/O space, so there is nothing invariant
; to hoist and PORT_HW_SETUP is empty.
;-----------------------------------------------------------------------------

PORT_HW_SETUP MACRO
ENDM

	; The decoder offers backends private stack words from [bp-18] down.
	; The BIOS tick needs no state beyond SI, so the PC asks for none --
	; which is what keeps the PC stack frame exactly as it was.
PORT_RX_FRAME_PUSH MACRO
ENDM

PORT_RX_FRAME_POP MACRO
ENDM

PORT_RX_STATUS_ADDR MACRO base
	mov	dx, base
	add	dx, UART_LSR_OFF
ENDM

PORT_RX_TEST MACRO ready
	in	al, dx
	test	al, LSR_DR
	jnz	ready
ENDM

PORT_RX_DATA MACRO base
	mov	dx, base
	add	dx, UART_RBR_OFF
	in	al, dx
ENDM

PORT_TX_WAIT MACRO base
	LOCAL wait_loop

wait_loop:
	mov	dx, base
	add	dx, UART_LSR_OFF
	in	al, dx
	test	al, LSR_THRE		; Check if transmitter ready
	jz	wait_loop
ENDM

PORT_TX_DATA MACRO base
	mov	dx, base
	add	dx, UART_THR_OFF
	out	dx, al
ENDM

;-----------------------------------------------------------------------------
; Timebase: the BIOS tick counter at 0040:006Ch, 18.2 Hz.  Timeouts arrive
; in milliseconds and are scaled to ticks once, up front.
;-----------------------------------------------------------------------------

PORT_TIME_SETUP MACRO
	mov	ax, BIOS_DATA_SEG
	mov	es, ax
ENDM

	; Convert a millisecond timeout in AX to ticks, in place.  CX is live
	; in the caller (buffer space remaining), hence the push.
PORT_TIME_SCALE MACRO timeout
	mov	ax, timeout
	xor	dx, dx
	push	cx
	mov	cx, 55
	div	cx			; AX = timeout in ticks
	pop	cx
	mov	timeout, ax		; Overwrite parameter with ticks
ENDM

PORT_TIME_MARK MACRO timeout
	mov	si, es:[BIOS_TICK_OFFSET]
ENDM

PORT_TIME_CHECK MACRO again, timeout
	sti
	mov	ax, es:[BIOS_TICK_OFFSET]
	sub	ax, si
	cmp	ax, timeout
	jb	again
ENDM

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

	PUBLIC	_port_init

;-----------------------------------------------------------------------------
; void port_init(uint16_t base, uint16_t divisor)
; Initialize the UART for specified baud rate, 8N1
; Parameters: base (UART base port address), divisor (baud rate divisor)
;-----------------------------------------------------------------------------
_port_init	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	dx

	; Save base address to global variable
	mov	ax, [bp+4]		; First parameter: base
	mov	_port_uart_base, ax
	mov	bx, [bp+6]		; Second parameter: divisor

	mov	dx, ax			; DX = base port address

	; Set DLAB to access divisor latch
	add	dx, UART_LCR_OFF
	mov	al, LCR_DLAB
	out	dx, al

	; Set baud rate using divisor parameter
	mov	dx, _port_uart_base
	add	dx, UART_DLL_OFF
	mov	al, bl			; Low byte of divisor
	out	dx, al
	mov	dx, _port_uart_base
	add	dx, UART_DLH_OFF
	mov	al, bh			; High byte of divisor
	out	dx, al

	; Set line control: 8N1, clear DLAB
	mov	dx, _port_uart_base
	add	dx, UART_LCR_OFF
	mov	al, LCR_8N1
	out	dx, al

	; Enable DTR, RTS, OUT2
	mov	dx, _port_uart_base
	add	dx, UART_MCR_OFF
	mov	al, MCR_DTR OR MCR_RTS OR MCR_OUT2
	out	dx, al

	; Disable interrupts
	mov	dx, _port_uart_base
	add	dx, UART_IER_OFF
	xor	al, al
	out	dx, al

	; Clear any pending data
	mov	dx, _port_uart_base
	add	dx, UART_RBR_OFF
	in	al, dx

	pop	dx
	pop	bx
	pop	ax
	pop	bp
	ret
_port_init	ENDP
