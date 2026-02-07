;-----------------------------------------------------------------------------
; int port_putc(uint8_t c)
; Send one character to the UART
; Parameters: c (byte) on stack
; Returns: Character sent in AX, or -1 on error
;-----------------------------------------------------------------------------
_port_putc	PROC	NEAR
	push	bp
	mov	bp, sp
	push	dx

putc_wait:
	mov	dx, _port_uart_base
	add	dx, UART_LSR_OFF
	in	al, dx
	test	al, LSR_THRE		; Check if transmitter ready
	jz	putc_wait

	; Send the character
	mov	al, [bp+4]		; Get character parameter
	mov	dx, _port_uart_base
	add	dx, UART_THR_OFF
	out	dx, al

	xor	ah, ah			; Return character in AX

	pop	dx
	pop	bp
	ret
_port_putc	ENDP
