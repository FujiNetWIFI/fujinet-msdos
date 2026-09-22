;-----------------------------------------------------------------------------
; ACT Apricot F1 / F1e / F2 / F10 - Zilog Z80 SIO channel B backend.
;
; Read the contract at the top of mach/pc.inc first.
;
; Hardware, from MAME src/mame/act/apricotf.cpp.  Despite the shared family
; name this machine is NOT the Apricot PC: different ports, the other SIO
; channel, and a Z80 CTC instead of an 8253.
;
;   The SIO is mapped .umask16(0x00ff) over 20h-27h with ba_cd decoding
;   (address bit 1 = channel, bit 0 = control/data), so on even CPU
;   addresses:
;
;     Z80 SIO   20h ch A data   22h ch A control/status
;               24h ch B data   26h ch B control/status
;     Z80 CTC   10h ch0  12h ch1  14h ch2  16h ch3
;
;   RS-232 is on channel B here, the opposite of the Apricot PC - the F1's
;   out_txdb/rtsb/dtrb are what reach the RS232 port.  Channel A serves the
;   Centronics handshake.
;
;   Baud: CTC channel 1 drives both RxCB and TxCB from a 153.846 kHz clock
;   (14 MHz / 7 / 13).  Used in COUNTER mode the CTC divides that input by
;   its time constant with no prescaler, and with the SIO in x16 mode that
;   gives N = 9615 / bps.  So 9600 needs N=1 (9615 baud, +0.16%) and 9600
;   is this machine's ceiling - there is simply no headroom above it.
;-----------------------------------------------------------------------------

	.data

_port_uart_base	DW	24h		; Z80 SIO channel B data register

	.code

	include mach/sio_regs.inc

	; Z80 SIO channel B, offsets from the data register
SIO_DATA_OFF	EQU	0		; Data (read: RX, write: TX)
SIO_CTRL_OFF	EQU	2		; Read: RRn, write: pointer then WRn

	; Z80 CTC channel 1, the channel B baud clock
CTC_CH1		EQU	12h
	; Counter mode, time constant follows, software reset, control word,
	; interrupts off.  Counter mode divides the input by the time
	; constant directly, bypassing the prescaler.
CTC_CTR_MODE	EQU	047h

;-----------------------------------------------------------------------------
; Timing.  No BIOS data area, so the receive timeout counts poll bursts;
; see the fuller explanation in mach/sanyo550.inc.  8086, so the same burst
; sizing as the Apricot PC.
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
; Program the baud clock and bring up SIO channel B for 8N1, polled.
; Parameters: base (SIO channel B data register), divisor (CTC time constant)
;-----------------------------------------------------------------------------

	; The SIO needs recovery time between consecutive register accesses.
IO_DELAY MACRO
	jmp	short $+2
	jmp	short $+2
ENDM

	; Write a value to channel B write register N.  DX = control port.
SIO_WR MACRO reg, value
	mov	al, reg
	out	dx, al
	IO_DELAY
	mov	al, value
	out	dx, al
	IO_DELAY
ENDM

	; Write a bare WR0 command.  DX = control port.
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

	; --- Baud: CTC channel 1 in counter mode, time constant = divisor.
	; --- A time constant of 0 means 256 to the CTC, so floor it at 1.
	mov	dx, CTC_CH1
	mov	al, CTC_CTR_MODE
	out	dx, al
	IO_DELAY
	mov	al, bl
	test	al, al
	jnz	tc_ok
	inc	al
tc_ok:
	out	dx, al
	IO_DELAY

	; --- SIO channel B: 8N1, polled, no auto enables ---
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
