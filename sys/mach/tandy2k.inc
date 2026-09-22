;-----------------------------------------------------------------------------
; Tandy 2000 - Intel 8251A USART backend.
;
; Read the contract at the top of mach/pc.inc first.  This backend is the
; Sanyo's chip at different addresses, plus a faster CPU that forces a
; two-level receive timeout.
;
; Hardware, from MAME src/mame/trs/tandy2k.cpp:
;   80186 at 8 MHz.  The i8251A is mapped .umask16(0x00ff) over 10h-13h, so
;   it answers on even addresses only:
;
;     i8251A    data 10h, status/control 12h   (mirrored at 1Ch/1Eh)
;     8253 PIT  counters 40h/42h/44h, control 46h
;
;   Baud: 8253 counter 1 drives both TxC and RxC through intbrclk_w, input
;   16 MHz / 8 = 2 MHz, with the 8251 in x16 mode.  Counter 0 is the
;   speaker.  9600 comes out at divisor 13 => 9615 baud, +0.16%.
;
; WHY THIS BACKEND DOES NOT SELECT THE INTERNAL BAUD CLOCK
; --------------------------------------------------------
; Bit 1 (EXTCLK) of the write-only control register at I/O 00h chooses
; between the internal 8253 baud clock and an external one, and we need it
; clear.  We deliberately leave it alone, because that register cannot be
; safely updated:
;
;   - It is write-only.  Reading 00h returns something else entirely (the
;     RS-232 ring indicator, carrier detect and _ACLOW), not a read-back of
;     the latch, so there is no read-modify-write.
;   - Its other bits control things we must not disturb by guessing:
;     bit 0 keyboard enable, bit 4 RFRQGATE which gates PIT counters 1 and
;     2 - counter 2 being the DRAM refresh pulse generator - and bit 5 the
;     8272 floppy reset.
;
; Clearing RFRQGATE would stop memory refresh and take the machine down, so
; guessing a whole byte here is not an acceptable risk for a driver.  The
; machine cannot have booted without RFRQGATE set and, since its own serial
; support uses the internal clock, EXTCLK clear.  We inherit both and only
; program counter 1's count.  If a Tandy 2000 ever turns up configured for
; an external clock, that is a documented bring-up step, not a driver bug.
;-----------------------------------------------------------------------------

	.data

_port_uart_base	DW	10h		; i8251A data register

	.code

	; i8251A register offsets, relative to the data register
I8251_DATA_OFF	EQU	0		; Data (read: RX, write: TX)
I8251_CTRL_OFF	EQU	2		; Read: status, write: mode/command

	; i8251A status register bits
I8251_ST_TXRDY	EQU	01h		; Transmitter ready for a byte
I8251_ST_RXRDY	EQU	02h		; Received byte available

	; i8251A mode instruction: x16 clock, 8 data bits, no parity, 1 stop
I8251_MODE_8N1	EQU	4Eh

	; i8251A command instructions
I8251_CMD_IRESET EQU	40h		; Internal reset, back to mode state
I8251_CMD_RUN	EQU	37h		; RxE|DTR|TxEN|ER|RTS, no break, no hunt

	; 8253 PIT
PIT_CTR1	EQU	42h
PIT_CTRL	EQU	46h
	; Counter 1, LSB then MSB, mode 3 (square wave), binary
PIT_CTR1_MODE3	EQU	76h

;-----------------------------------------------------------------------------
; Timing.
;
; No BIOS tick counter here either, so the timeout counts poll iterations.
; Unlike the Sanyo this machine needs TWO levels: an 80186 at 8 MHz runs the
; poll loop in roughly 3.5 us, so a full 255-poll burst is only about 0.9 ms
; and a 16-bit burst count would overflow well before the 120000 ms
; FUJICMD_COPY_FILE timeout (120000 / 0.9 = 133000 bursts).
;
; So SI counts bursts within a unit, and one backend stack word counts
; units.  PORT_MS_PER_UNIT is an under-estimate on purpose: too long only
; costs time on a dead link, too short fails live transfers.
;   255 polls * 3.5 us   ~= 0.9 ms per burst
;   16 bursts            ~= 14 ms per unit, claimed as 8
;   120000 / 8           =  15000 units, fits comfortably
;-----------------------------------------------------------------------------
PORT_POLL_BURST	EQU	255
PORT_BURSTS_PER_UNIT EQU 16
PORT_MS_PER_UNIT EQU	8

	; Backend stack word: units remaining.  The decoder hands out words
	; from [bp-18] down; see the contract in mach/pc.inc.
SLIPD_LOCAL_TMO_UNITS EQU [bp-18]

;-----------------------------------------------------------------------------
; Register access.  I/O space; ES untouched.
;-----------------------------------------------------------------------------

PORT_HW_SETUP MACRO
ENDM

PORT_RX_FRAME_PUSH MACRO
	xor	ax, ax
	push	ax			; [bp-18] units remaining
ENDM

PORT_RX_FRAME_POP MACRO
	pop	ax
ENDM

PORT_RX_STATUS_ADDR MACRO base
	mov	dx, base
	add	dx, I8251_CTRL_OFF
ENDM

PORT_RX_TEST MACRO ready
	in	al, dx
	test	al, I8251_ST_RXRDY
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
	add	dx, I8251_CTRL_OFF
	in	al, dx
	test	al, I8251_ST_TXRDY
	jz	wait_loop
ENDM

PORT_TX_DATA MACRO base
	mov	dx, base
	out	dx, al
ENDM

;-----------------------------------------------------------------------------
; Timebase: bursts within units.  ES is deliberately not used.
;-----------------------------------------------------------------------------

PORT_TIME_SETUP MACRO
ENDM

	; Convert a millisecond timeout to a unit count, in place.  Round up
	; and floor at 1.  CX is live in the caller, hence the push.
PORT_TIME_SCALE MACRO timeout
	LOCAL not_zero

	mov	ax, timeout
	add	ax, PORT_MS_PER_UNIT - 1
	xor	dx, dx
	push	cx
	mov	cx, PORT_MS_PER_UNIT
	div	cx			; AX = timeout in units
	pop	cx
	test	ax, ax
	jnz	not_zero
	inc	ax
not_zero:
	mov	timeout, ax
ENDM

PORT_TIME_MARK MACRO timeout
	mov	si, PORT_BURSTS_PER_UNIT
	mov	ax, timeout
	mov	SLIPD_LOCAL_TMO_UNITS, ax
ENDM

PORT_TIME_CHECK MACRO again, timeout
	LOCAL next_unit

	sti
	dec	si
	jnz	again
	mov	si, PORT_BURSTS_PER_UNIT
	dec	word ptr SLIPD_LOCAL_TMO_UNITS
	jnz	again
ENDM

	PUBLIC	_port_init

;-----------------------------------------------------------------------------
; void port_init(uint16_t base, uint16_t divisor)
; Program the baud clock and bring up the 8251A for 8N1.
; Parameters: base (8251A data register), divisor (8253 counter 1 count)
;
; Does NOT touch the control register at I/O 00h - see the note at the top
; of this file.
;-----------------------------------------------------------------------------

	; The 8251A needs recovery time between consecutive control writes.
IO_DELAY MACRO
	jmp	short $+2
	jmp	short $+2
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

	; Baud rate: 8253 counter 1, mode 3, 16-bit count
	mov	dx, PIT_CTRL
	mov	al, PIT_CTR1_MODE3
	out	dx, al
	IO_DELAY
	mov	dx, PIT_CTR1
	mov	al, bl			; Low byte of count
	out	dx, al
	IO_DELAY
	mov	al, bh			; High byte of count
	out	dx, al
	IO_DELAY

	; Force the 8251A into a known state: three zero writes satisfy any
	; pending mode or sync-character wait, then the internal reset
	; command puts it back into the mode-instruction state.
	mov	dx, _port_uart_base
	add	dx, I8251_CTRL_OFF
	xor	al, al
	out	dx, al
	IO_DELAY
	out	dx, al
	IO_DELAY
	out	dx, al
	IO_DELAY
	mov	al, I8251_CMD_IRESET
	out	dx, al
	IO_DELAY

	; Mode instruction, then command
	mov	al, I8251_MODE_8N1
	out	dx, al
	IO_DELAY
	mov	al, I8251_CMD_RUN
	out	dx, al
	IO_DELAY

	; Clear any pending data
	mov	dx, _port_uart_base
	in	al, dx

	pop	dx
	pop	bx
	pop	ax
	pop	bp
	ret
_port_init	ENDP
