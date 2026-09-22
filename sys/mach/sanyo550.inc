;-----------------------------------------------------------------------------
; Sanyo MBC-550 / MBC-555 - Intel 8251A USART backend.
;
; Read the contract at the top of mach/pc.inc first.
;
; Hardware, from MAME src/mame/sanyo/mbc55x.cpp:
;   The machine's I/O decode passes address bits 1..5 through and drops
;   bit 0 (iodecode_r does read8(offset >> 1)), so every peripheral appears
;   at twice the address its chip select suggests, on even addresses.
;
;     i8251A (RS-232)    data 28h, status/control 2Ah  (chip select 14h/15h)
;     i8251A (keyboard)  data 38h, status/control 3Ah  - DO NOT TOUCH
;     8253 PIT           counters 20h/22h/24h, control 26h
;     8259 PIC           00h, 02h
;
;   The RS-232 8251 is on a separate board through a 20-pin header, so a
;   machine without that board has nothing at 28h.
;
;   Baud: 8253 counter 2 drives both TxC and RxC, input 14.318181 MHz / 8 =
;   1.7898 MHz, with the 8251 in x16 mode.  Counter 0 is the system tick and
;   counter 1 is chained off it, so counter 2 is ours to program.
;
;   9600 works out at divisor 12 => 9322 baud, -2.9% off.  That is inside an
;   8N1 frame's budget but it is the worst of the supported machines, so try
;   FUJI_BPS=4800 (divisor 23, +1.3%) if a link proves flaky.
;-----------------------------------------------------------------------------

	.data

; Default to the RS-232 board's 8251 data register
_port_uart_base	DW	28h

	.code

	; i8251A register offsets, relative to the data register
I8251_DATA_OFF	EQU	0		; Data (read: RX, write: TX)
I8251_CTRL_OFF	EQU	2		; Read: status, write: mode/command

	; i8251A status register bits
I8251_ST_TXRDY	EQU	01h		; Transmitter ready for a byte
I8251_ST_RXRDY	EQU	02h		; Received byte available
I8251_ST_TXEMPTY EQU	04h		; Transmitter drained

	; i8251A mode instruction: x16 clock, 8 data bits, no parity, 1 stop
I8251_MODE_8N1	EQU	4Eh

	; i8251A command instructions
I8251_CMD_IRESET EQU	40h		; Internal reset, back to mode state
I8251_CMD_RUN	EQU	37h		; RxE|DTR|TxEN|ER|RTS, no break, no hunt

	; 8253 PIT, after the machine's address shift
PIT_CTR2	EQU	24h
PIT_CTRL	EQU	26h
	; Counter 2, LSB then MSB, mode 3 (square wave), binary
PIT_CTR2_MODE3	EQU	0B6h

;-----------------------------------------------------------------------------
; Timing.
;
; There is no BIOS tick counter at 0040:006Ch here, so the receive timeout
; counts poll iterations instead: PORT_POLL_BURST polls per burst, and SI
; counts bursts.
;
; PORT_MS_PER_BURST must be an UNDER-estimate of how long a burst really
; takes.  Under-estimating makes timeouts longer than nominal, which only
; costs time when a link is already dead; over-estimating would cut them
; short and fail live transfers.  It must also be at least 2, or the
; 120000 ms FUJICMD_COPY_FILE timeout overflows the 16-bit burst count.
;
; 8088 at 3.58 MHz: the four-instruction poll loop runs at roughly 13 us an
; iteration once 8-bit-bus prefetch starvation is counted, so 200 polls is
; about 2.6 ms.  Claimed as 2.
;-----------------------------------------------------------------------------
PORT_POLL_BURST	EQU	200
PORT_MS_PER_BURST EQU	2

;-----------------------------------------------------------------------------
; Register access.  I/O space, so nothing invariant to hoist, and ES is
; untouched - which is why the timebase above may not use it either.
;-----------------------------------------------------------------------------

PORT_HW_SETUP MACRO
ENDM

PORT_RX_FRAME_PUSH MACRO
ENDM

PORT_RX_FRAME_POP MACRO
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
; Timebase: count bursts of polls.  ES is deliberately not used.
;-----------------------------------------------------------------------------

PORT_TIME_SETUP MACRO
ENDM

	; Convert a millisecond timeout to a burst count, in place.  Round up
	; and floor at 1 so a small timeout never becomes "no wait at all".
	; CX is live in the caller, hence the push.
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
; Program the baud clock and bring up the 8251A for 8N1.
; Parameters: base (8251A data register), divisor (8253 counter 2 count)
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

	; Baud rate: 8253 counter 2, mode 3, 16-bit count
	mov	dx, PIT_CTRL
	mov	al, PIT_CTR2_MODE3
	out	dx, al
	IO_DELAY
	mov	dx, PIT_CTR2
	mov	al, bl			; Low byte of count
	out	dx, al
	IO_DELAY
	mov	al, bh			; High byte of count
	out	dx, al
	IO_DELAY

	; The 8251A has no reset line we can pull, and after power-up it may
	; still be waiting for mode or sync-character bytes.  Three zero
	; writes satisfy any such wait, after which the internal reset
	; command puts it back into the mode-instruction state for certain.
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

	; Now the real configuration: mode instruction, then command
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
