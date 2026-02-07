;-----------------------------------------------------------------------------
; int port_getc(void)
; Read one character from the UART if available
; Returns: Character in AX (0-255), or -1 if no data available
;-----------------------------------------------------------------------------
_port_getc	PROC	NEAR
	push	dx

	mov	dx, _port_uart_base
	add	dx, UART_LSR_OFF
	in	al, dx
	test	al, LSR_DR		; Check if data ready
	jz	getc_no_data

	; Read the character
	mov	dx, _port_uart_base
	add	dx, UART_RBR_OFF
	in	al, dx
	xor	ah, ah			; Zero extend to word
	jmp	getc_done

getc_no_data:
	mov	ax, -1			; No data available

getc_done:
	pop	dx
	ret
_port_getc	ENDP
