;-----------------------------------------------------------------------------
; Zilog Z80 SIO / NEC uPD7201 MPSC register values.
;
; Shared by every SIO-family backend: victor9k (uPD7201, memory mapped),
; apricot (Z80 SIO channel A) and apricotf (Z80 SIO channel B).  Only the
; values live here - how you reach the registers is the backend's business,
; because one of these machines has no I/O address space.
;
; Programming model: write a register number to the control port, then the
; value.  Status reads come back as RR0, and the register pointer
; auto-resets to 0 after every control read, so a poll loop can re-read the
; control port indefinitely and keep getting RR0.
;-----------------------------------------------------------------------------

	; RR0 status bits
RR0_RX_AVAIL	EQU	01h		; Received character available
RR0_TX_EMPTY	EQU	04h		; Transmit buffer empty

	; WR0 commands (the pointer bits select register 0)
WR0_CH_RESET	EQU	018h		; Channel reset
WR0_RST_EXT	EQU	010h		; Reset external/status interrupts
WR0_ERR_RESET	EQU	030h		; Error reset

	; Write register values for a polled, 8N1, no-flow-control channel.
	;
	; WR3 bit 5 (AUTO ENABLES, 20h) is deliberately clear.  With it set
	; the chip gates receive on DCD and transmit on CTS; a FujiNet
	; adapter asserts neither, which would deadlock the port.  The Victor
	; boot ROM uses WR3=C1h for exactly this reason.
WR1_POLLED	EQU	004h		; Status affects vector, no interrupts
WR2_8086_NONVEC	EQU	010h		; 8086/8088 mode, non-vectored
WR4_8N1_X16	EQU	044h		; x16 clock, 1 stop bit, no parity
WR3_RX_8BIT	EQU	0C1h		; RX 8 bits/char, RX enable, no auto enables
WR5_TX_8BIT	EQU	0EAh		; DTR, TX 8 bits/char, TX enable, RTS
