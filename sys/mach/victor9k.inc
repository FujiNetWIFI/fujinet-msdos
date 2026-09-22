;-----------------------------------------------------------------------------
; Victor 9000 / ACT Sirius 1 - NEC uPD7201 MPSC backend.
;
; Read the contract at the top of mach/pc.inc first.  This is the only
; backend whose registers are MEMORY MAPPED: the Victor has no I/O address
; space at all, so in/out reach nothing and every access is a mov through
; ES.  That is what PORT_HW_SETUP and PORT_NEEDS_ES_FOR_HW exist for.
;
; Hardware, from MAME src/mame/act/victor9k.cpp, cross-checked against the
; Victor's own boot ROM (which programs channel A at linear 0F822Bh):
;
;     uPD7201 MPSC   E004:0000 ch A data   E004:0002 ch A control/status
;                    E004:0001 ch B data   E004:0003 ch B control/status
;     8253 PIT       E002:0000 ctr0  E002:0001 ctr1  E002:0002 ctr2
;                    E002:0003 control
;     6522 VIA2      E804:0000 ORB  E804:0001 ORA  E804:0002 DDRB
;                    E804:0003 DDRA
;
;   The MPSC is an 8274 derivative, so the programming model is Z80 SIO:
;   write a register number to the control port then the value, and read
;   status back as RR0.  The register pointer auto-resets to 0 after every
;   control read, so the poll loop can re-read the control port forever and
;   keep getting RR0.
;
;   Baud: 8253 counter 0 drives both RxCA and TxCA, input 15 MHz / 12 =
;   1.25 MHz, with the MPSC in x16 mode, so N = 78125 / bps.  Counter 1 is
;   channel B's clock and counter 2 is the system tick, so counter 0 is
;   ours.  Note counter 0 clocks ONLY channel A - channel B keeps its own
;   rate.  9600 gives N=8 => 9766 baud, +1.7%; 19200 and 38400 also work.
;
;-----------------------------------------------------------------------------

	.data

; Segment of the MPSC registers.  Not an I/O address: see above.
_port_uart_base	DW	0E004h

	.code

	include mach/sio_regs.inc

	; uPD7201 channel A, offsets within the MPSC segment
MPSC_DATA_A	EQU	0		; Data (read: RX, write: TX)
MPSC_CTRL_A	EQU	2		; Read: RRn, write: pointer then WRn

	; 8253 PIT, in its own segment
PIT_SEG		EQU	0E002h
PIT_CTR0	EQU	0
PIT_CTRL	EQU	3
	; Counter 0, LSB then MSB, mode 3 (square wave), binary.  Same
	; control byte the boot ROM uses.
PIT_CTR0_MODE3	EQU	036h

	; 6522 VIA2, in its own segment.  PA0/PA1 are _INT/EXTA and
	; _INT/EXTB: driving them low selects the internal (8253) clocks.
	; MAME stubs this write, but it is real on hardware.
VIA2_SEG	EQU	0E804h
VIA2_ORA	EQU	1
VIA2_DDRA	EQU	3
VIA2_CLKSEL	EQU	003h		; PA0 | PA1

;-----------------------------------------------------------------------------
; Timing.
;
; No BIOS data area here, so the receive timeout counts poll iterations:
; PORT_POLL_BURST polls per burst, SI counts bursts.  One level is enough
; because an 8088 at 5 MHz takes roughly 11 us per memory-mapped poll, so
; 200 polls is about 2.2 ms and the 120000 ms FUJICMD_COPY_FILE timeout
; needs only 60000 bursts.
;
; PORT_MS_PER_BURST is an under-estimate on purpose: too long merely slows
; error recovery, too short would fail live transfers.  It must be at least
; 2 or that 120000 ms timeout overflows the 16-bit burst count.
;-----------------------------------------------------------------------------
PORT_POLL_BURST	EQU	200
PORT_MS_PER_BURST EQU	2

;-----------------------------------------------------------------------------
; Register access.  ES holds the MPSC segment for the whole routine, which
; is why the timebase above must not use ES.
;-----------------------------------------------------------------------------

PORT_NEEDS_ES_FOR_HW EQU 1

PORT_HW_SETUP MACRO
	mov	ax, _port_uart_base
	mov	es, ax
ENDM

PORT_RX_FRAME_PUSH MACRO
ENDM

PORT_RX_FRAME_POP MACRO
ENDM

	; Nothing to hoist: the registers are at fixed offsets from ES.
PORT_RX_STATUS_ADDR MACRO base
ENDM

PORT_RX_TEST MACRO ready
	mov	al, es:[MPSC_CTRL_A]	; RR0; pointer auto-resets, so repeatable
	test	al, RR0_RX_AVAIL
	jnz	ready
ENDM

PORT_RX_DATA MACRO base
	mov	al, es:[MPSC_DATA_A]
ENDM

PORT_TX_WAIT MACRO base
	LOCAL wait_loop

wait_loop:
	mov	al, es:[MPSC_CTRL_A]	; RR0
	test	al, RR0_TX_EMPTY
	jz	wait_loop
ENDM

PORT_TX_DATA MACRO base
	mov	es:[MPSC_DATA_A], al
ENDM

;-----------------------------------------------------------------------------
; Timebase: count bursts of polls.  ES belongs to the MPSC, not to us.
;-----------------------------------------------------------------------------

PORT_TIME_SETUP MACRO
ENDM

	; Convert a millisecond timeout to a burst count, in place.  Round up
	; and floor at 1.  CX is live in the caller, hence the push.
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
; Program the baud clock and bring up MPSC channel A for 8N1, polled.
; Parameters: base (MPSC register SEGMENT), divisor (8253 counter 0 count)
;-----------------------------------------------------------------------------

	; The MPSC needs recovery time between consecutive register accesses.
IO_DELAY MACRO
	jmp	short $+2
	jmp	short $+2
ENDM

	; Write a value to MPSC channel A write register N: point at it with
	; a WR0 write, then supply the value.  ES must hold the MPSC segment.
MPSC_WR MACRO reg, value
	mov	al, reg
	mov	es:[MPSC_CTRL_A], al
	IO_DELAY
	mov	al, value
	mov	es:[MPSC_CTRL_A], al
	IO_DELAY
ENDM

	; Write a bare WR0 command (the pointer bits select register 0).
MPSC_CMD MACRO value
	mov	al, value
	mov	es:[MPSC_CTRL_A], al
	IO_DELAY
ENDM

_port_init	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	dx
	push	es

	mov	ax, [bp+4]		; First parameter: MPSC segment
	mov	_port_uart_base, ax
	mov	bx, [bp+6]		; Second parameter: divisor

	; --- Baud rate: 8253 counter 0, mode 3, 16-bit count ---
	mov	ax, PIT_SEG
	mov	es, ax
	mov	al, PIT_CTR0_MODE3
	mov	es:[PIT_CTRL], al
	IO_DELAY
	mov	al, bl			; Low byte of count
	mov	es:[PIT_CTR0], al
	IO_DELAY
	mov	al, bh			; High byte of count
	mov	es:[PIT_CTR0], al
	IO_DELAY

	; --- Select the internal (8253) clocks via VIA2 PA0/PA1 ---
	mov	ax, VIA2_SEG
	mov	es, ax
	mov	al, es:[VIA2_DDRA]
	or	al, VIA2_CLKSEL		; PA0/PA1 become outputs
	mov	es:[VIA2_DDRA], al
	IO_DELAY
	mov	al, es:[VIA2_ORA]
	and	al, NOT VIA2_CLKSEL	; drive them low = internal
	mov	es:[VIA2_ORA], al
	IO_DELAY

	; --- MPSC channel A: 8N1, polled, no auto enables ---
	mov	ax, _port_uart_base
	mov	es, ax

	MPSC_CMD WR0_CH_RESET
	MPSC_WR	2, WR2_8086_NONVEC
	MPSC_WR	4, WR4_8N1_X16		; WR4 first: it sets the clock mode
	MPSC_WR	3, WR3_RX_8BIT
	MPSC_WR	5, WR5_TX_8BIT
	MPSC_CMD WR0_RST_EXT
	MPSC_CMD WR0_ERR_RESET
	MPSC_WR	1, WR1_POLLED

	; Clear any pending data
	mov	al, es:[MPSC_DATA_A]

	pop	es
	pop	dx
	pop	bx
	pop	ax
	pop	bp
	ret
_port_init	ENDP
