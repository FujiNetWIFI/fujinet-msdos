;-----------------------------------------------------------------------------
; int port_getc_timeout(uint16_t timeout)
; Wait for a character with timeout in milliseconds
; Parameters: timeout in stack (milliseconds)
; Returns: Character in AX (0-255), or -1 on timeout
;-----------------------------------------------------------------------------
_port_getc_timeout PROC NEAR
	push	bp
	mov	bp, sp
	push	bx
	push	cx
	push	dx
	push	es

	; Read starting BIOS tick count (0040:006C)
	mov	ax, 40h
	mov	es, ax
	mov	bx, es:[6Ch]		; Get current tick count

	; Convert timeout from ms to ticks (timeout / 55)
	mov	ax, [bp+4]		; Get timeout parameter in ms
	mov	cx, 55
	xor	dx, dx
	div	cx			; AX = timeout in ticks
	mov	cx, ax			; CX = timeout in ticks
	add	cx, bx			; CX = end tick count

getct_check:
	push	cx
	mov	dx, _port_uart_base
	add	dx, UART_LSR_OFF
	in	al, dx
	test	al, LSR_DR		; Check if data ready
	pop	cx
	jnz	getct_got_char

	; Check if timeout expired
	push	cx
	mov	ax, 40h
	mov	es, ax
	mov	ax, es:[6Ch]		; Get current tick count
	pop	cx
	cmp	ax, cx			; Compare current to end time
	jb	getct_check		; Continue if not expired

	; Timeout occurred
	mov	ax, -1
	jmp	getct_done

getct_got_char:
	; Read the character
	mov	dx, _port_uart_base
	add	dx, UART_RBR_OFF
	in	al, dx
	xor	ah, ah			; Zero extend to word

getct_done:
	pop	es
	pop	dx
	pop	cx
	pop	bx
	pop	bp
	ret
_port_getc_timeout ENDP
