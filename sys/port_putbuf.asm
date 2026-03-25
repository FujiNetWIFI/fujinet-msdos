;-----------------------------------------------------------------------------
; uint16_t port_putbuf(void *buf, uint16_t len)
; Send multiple characters from buffer
; Parameters: buf (pointer), len (word)
; Returns: Number of characters sent in AX
;-----------------------------------------------------------------------------
_port_putbuf	PROC	NEAR
	push	bp
	mov	bp, sp
	push	bx
	push	cx
	push	dx
	push	si
	push	ds

	mov	si, [bp+4]		; Get buffer pointer
	mov	cx, [bp+6]		; CX = length (countdown)
	mov	bx, cx			; BX = original length

putb_send_loop:
	test	cx, cx			; Check if count reached zero
	jz	putb_done

putb_wait:
	mov	dx, _port_uart_base
	add	dx, UART_LSR_OFF
	in	al, dx
	test	al, LSR_THRE		; Check if transmitter ready
	jz	putb_wait

	; Send character
	lodsb				; Load char from [DS:SI] and increment SI
	mov	dx, _port_uart_base
	add	dx, UART_THR_OFF
	out	dx, al

	dec	cx
	jmp	putb_send_loop

putb_done:
	mov	ax, bx			; Return original length (all sent)

	pop	ds
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	bp
	ret
_port_putbuf	ENDP
