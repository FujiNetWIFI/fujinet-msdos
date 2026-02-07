;-----------------------------------------------------------------------------
; void port_init(uint16_t base, uint16_t divisor)
; Initialize the UART for specified baud rate, 8N1
; Parameters: base (UART base port address), divisor (baud rate divisor)
;-----------------------------------------------------------------------------
_port_init	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	dx

	; Save base address to global variable
	mov	ax, [bp+4]		; First parameter: base
	mov	_port_uart_base, ax
	mov	bx, [bp+6]		; Second parameter: divisor

	mov	dx, ax			; DX = base port address

	; Set DLAB to access divisor latch
	add	dx, UART_LCR_OFF
	mov	al, LCR_DLAB
	out	dx, al

	; Set baud rate using divisor parameter
	mov	dx, _port_uart_base
	add	dx, UART_DLL_OFF
	mov	al, bl			; Low byte of divisor
	out	dx, al
	mov	dx, _port_uart_base
	add	dx, UART_DLH_OFF
	mov	al, bh			; High byte of divisor
	out	dx, al

	; Set line control: 8N1, clear DLAB
	mov	dx, _port_uart_base
	add	dx, UART_LCR_OFF
	mov	al, LCR_8N1
	out	dx, al

	; Enable DTR, RTS, OUT2
	mov	dx, _port_uart_base
	add	dx, UART_MCR_OFF
	mov	al, MCR_DTR OR MCR_RTS OR MCR_OUT2
	out	dx, al

	; Disable interrupts
	mov	dx, _port_uart_base
	add	dx, UART_IER_OFF
	xor	al, al
	out	dx, al

	; Clear any pending data
	mov	dx, _port_uart_base
	add	dx, UART_RBR_OFF
	in	al, dx

	pop	dx
	pop	bx
	pop	ax
	pop	bp
	ret
_port_init	ENDP
