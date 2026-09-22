;-----------------------------------------------------------------------------
; ACT Apricot PC / Xi - Zilog Z80 SIO channel A backend.
;
; Read the contract at the top of mach/pc.inc first.
;
; Hardware, from MAME src/mame/act/apricot.cpp.  The SIO is mapped as four
; separate byte ports rather than the usual ba_cd pair:
;
;     Z80 SIO   60h ch A data   62h ch A control/status
;               64h ch B data   66h ch B control/status
;     8253 PIT  58h ctr0  5Ah ctr1  5Ch ctr2  5Eh control
;     8259 PIC  00h, 02h
;
;   Channel B is the KEYBOARD (and the Centronics handshake).  This backend
;   never touches it - see below.
;
;   Baud: channel A's RXCA and TXCA come from a 74LS153 multiplexer, whose
;   inputs are 8253 counters 1 and 2 (both clocked at 4 MHz / 2 = 2 MHz)
;   and whose two select lines are driven by SIO channel B's DTR and RTS
;   outputs.  With the SIO in x16 mode, N = 125000 / bps; 9600 gives N=13
;   => 9615 baud, +0.16%.
;
; WHY THIS BACKEND PROGRAMS BOTH PIT COUNTER 1 AND COUNTER 2
; ----------------------------------------------------------
; The obvious way to control channel A's clock is to set the mux select,
; but those select lines are channel B's DTR and RTS, which means writing
; channel B's WR5 - and a Z80 SIO write register cannot be read back, so
; there is no read-modify-write and we would be guessing a whole byte on
; the keyboard's channel.
;
; It turns out we do not have to.  The mux wiring is:
;
;     select 0:  RXCA = counter 1,  TXCA = counter 2
;     select 2:  RXCA = counter 2,  TXCA = counter 2
;     select 1, 3: inputs not connected
;
; So if counters 1 and 2 are both programmed to the same rate, channel A
; ends up correctly clocked under either select setting, and we can leave
; the mux - and therefore channel B - completely alone.  Selects 1 and 3
; would leave channel A with no clock at all, so the machine's own firmware
; cannot be using them either.
;
; Counter 0 is the 250 kHz system tick feeding PIC IR6; we do not touch it.
; Counters 1 and 2 drive nothing but the mux, so they are ours.
;-----------------------------------------------------------------------------

	.data

_port_uart_base	DW	60h		; Z80 SIO channel A data register

	.code

	include mach/sio_regs.inc

	; Z80 SIO channel A, offsets from the data register
SIO_DATA_OFF	EQU	0		; Data (read: RX, write: TX)
SIO_CTRL_OFF	EQU	2		; Read: RRn, write: pointer then WRn

	; 8253 PIT
PIT_CTR1	EQU	5Ah
PIT_CTR2	EQU	5Ch
PIT_CTRL	EQU	5Eh
	; LSB then MSB, mode 3 (square wave), binary
PIT_CTR1_MODE3	EQU	076h
PIT_CTR2_MODE3	EQU	0B6h

;-----------------------------------------------------------------------------
; Timing.  No BIOS data area, so the receive timeout counts poll bursts;
; see the fuller explanation in mach/sanyo550.inc.  8086 at 5 MHz, so the
; same burst sizing as the Victor.
;-----------------------------------------------------------------------------
PORT_POLL_BURST	EQU	200
PORT_MS_PER_BURST EQU	2

;-----------------------------------------------------------------------------
; Register access.  I/O space; ES untouched.
;-----------------------------------------------------------------------------

PORT_HW_SETUP MACRO
ENDM

PORT_RX_FRAME_PUSH MACRO
ENDM

PORT_RX_FRAME_POP MACRO
ENDM

PORT_RX_STATUS_ADDR MACRO base
	mov	dx, base
	add	dx, SIO_CTRL_OFF
ENDM

PORT_RX_TEST MACRO ready
	in	al, dx			; RR0; pointer auto-resets, so repeatable
	test	al, RR0_RX_AVAIL
	jnz	ready
ENDM

PORT_RX_DATA MACRO base
	mov	dx, base
	in	al, dx
ENDM

PORT_TX_WAIT MACRO base
	LOCAL wait_loop

wait_loop:
	mov	dx, base
	add	dx, SIO_CTRL_OFF
	in	al, dx			; RR0
	test	al, RR0_TX_EMPTY
	jz	wait_loop
ENDM

PORT_TX_DATA MACRO base
	mov	dx, base
	out	dx, al
ENDM

;-----------------------------------------------------------------------------
; Timebase: count bursts of polls.  ES is deliberately not used.
;-----------------------------------------------------------------------------

PORT_TIME_SETUP MACRO
ENDM

PORT_TIME_SCALE MACRO timeout
	LOCAL not_zero

	mov	ax, timeout
	add	ax, PORT_MS_PER_BURST - 1
	xor	dx, dx
	push	cx
	mov	cx, PORT_MS_PER_BURST
	div	cx			; AX = timeout in bursts
	pop	cx
	test	ax, ax
	jnz	not_zero
	inc	ax
not_zero:
	mov	timeout, ax
ENDM

PORT_TIME_MARK MACRO timeout
	mov	si, timeout
ENDM

PORT_TIME_CHECK MACRO again, timeout
	sti
	dec	si
	jnz	again
ENDM

	PUBLIC	_port_init

;-----------------------------------------------------------------------------
; void port_init(uint16_t base, uint16_t divisor)
; Program the baud clocks and bring up SIO channel A for 8N1, polled.
; Parameters: base (SIO channel A data register), divisor (PIT count)
;-----------------------------------------------------------------------------

	; The SIO needs recovery time between consecutive register accesses.
IO_DELAY MACRO
	jmp	short $+2
	jmp	short $+2
ENDM

	; Write a value to channel A write register N: point at it with a WR0
	; write, then supply the value.  DX must hold the control port.
SIO_WR MACRO reg, value
	mov	al, reg
	out	dx, al
	IO_DELAY
	mov	al, value
	out	dx, al
	IO_DELAY
ENDM

	; Write a bare WR0 command.  DX must hold the control port.
SIO_CMD MACRO value
	mov	al, value
	out	dx, al
	IO_DELAY
ENDM

_port_init	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	dx

	mov	ax, [bp+4]		; First parameter: base
	mov	_port_uart_base, ax
	mov	bx, [bp+6]		; Second parameter: divisor

	; --- Baud: program BOTH counter 1 and counter 2, so channel A is
	; --- correctly clocked whichever way the mux is already set.
	mov	dx, PIT_CTRL
	mov	al, PIT_CTR1_MODE3
	out	dx, al
	IO_DELAY
	mov	dx, PIT_CTR1
	mov	al, bl
	out	dx, al
	IO_DELAY
	mov	al, bh
	out	dx, al
	IO_DELAY

	mov	dx, PIT_CTRL
	mov	al, PIT_CTR2_MODE3
	out	dx, al
	IO_DELAY
	mov	dx, PIT_CTR2
	mov	al, bl
	out	dx, al
	IO_DELAY
	mov	al, bh
	out	dx, al
	IO_DELAY

	; --- SIO channel A: 8N1, polled, no auto enables ---
	mov	dx, _port_uart_base
	add	dx, SIO_CTRL_OFF

	SIO_CMD	WR0_CH_RESET
	SIO_WR	4, WR4_8N1_X16		; WR4 first: it sets the clock mode
	SIO_WR	3, WR3_RX_8BIT
	SIO_WR	5, WR5_TX_8BIT
	SIO_CMD	WR0_RST_EXT
	SIO_CMD	WR0_ERR_RESET
	SIO_WR	1, WR1_POLLED

	; Clear any pending data
	mov	dx, _port_uart_base
	in	al, dx

	pop	dx
	pop	bx
	pop	ax
	pop	bp
	ret
_port_init	ENDP
