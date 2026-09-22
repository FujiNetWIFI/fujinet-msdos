	PUBLIC	_port_putc

;-----------------------------------------------------------------------------
; int port_putc(uint8_t c)
; Send one character to the serial port
; Parameters: c (byte) on stack
; Returns: Character sent in AX, or -1 on error
;-----------------------------------------------------------------------------
_port_putc	PROC	NEAR
	push	bp
	mov	bp, sp
	push	dx
IFDEF PORT_NEEDS_ES_FOR_HW
	push	es
ENDIF
	PORT_HW_SETUP

	PORT_TX_WAIT _port_uart_base

	; Send the character
	mov	al, [bp+4]		; Get character parameter
	PORT_TX_DATA _port_uart_base

	xor	ah, ah			; Return character in AX

IFDEF PORT_NEEDS_ES_FOR_HW
	pop	es
ENDIF
	pop	dx
	pop	bp
	ret
_port_putc	ENDP
