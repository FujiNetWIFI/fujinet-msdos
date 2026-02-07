;-----------------------------------------------------------------------------
; uint16_t port_getbuf(void *buf, uint16_t len, uint16_t timeout)
; Read multiple characters into buffer with timeout in milliseconds
; Parameters: buf (pointer), len (word), timeout (word in ms)
; Returns: Number of characters actually read in AX
;
; Register usage:
;   AX = scratch (UART data, calculations)
;   BX = timeout in ticks (constant)
;   CX = remaining characters to read (counts down to 0)
;   DX = UART port addresses
;   DI = buffer pointer (auto-incremented by stosb)
;   SI = end tick count for current character timeout
;   BP = stack frame pointer
;   ES = segment 40h (BIOS data area for tick counter)
;-----------------------------------------------------------------------------
_port_getbuf	PROC	NEAR
	push	bp
	mov	bp, sp
	push	bx
	push	cx
	push	dx
	push	di
	push	si
	push	es

	mov	di, [bp+4]		; Get buffer pointer
	mov	cx, [bp+6]		; CX = requested length (countdown)

	; Handle zero length case immediately
	test	cx, cx
	jz	getb_done

	; Convert timeout from ms to ticks once (timeout / 55)
	mov	ax, [bp+8]		; Get timeout parameter in ms
	xor	dx, dx
	push	cx
	mov	cx, 55
	div	cx			; AX = timeout in ticks
	pop	cx
	mov	bx, ax			; BX = timeout in ticks

	push	cx			; Save original length on stack

	; Set ES to BIOS data segment for tick counter access
	mov	ax, 40h
	mov	es, ax

getb_read_loop:
	; Get start time for this character
	mov	si, es:[6Ch]		; SI = start tick count
	add	si, bx			; SI = end tick count
	mov	dx, _port_uart_base
	add	dx, UART_LSR_OFF

getb_wait_char:
	cli
	mov	ah, 32

getb_skip_timeout:
	in	al, dx
	test	al, LSR_DR
	jnz	getb_got_char

	dec	ah
	jnz	getb_skip_timeout	; Don't need to check timeout constantly

	; Check if timeout expired
	sti
	mov	ax, es:[6Ch]		; Get current tick count
	cmp	ax, si			; Compare current to end time
	jb	getb_wait_char		; Continue if not expired

	; Timeout - return what we have
	jmp	getb_done

getb_got_char:
	mov	dx, _port_uart_base
	add	dx, UART_RBR_OFF
	in	al, dx
	mov	ds:[di], al		; Write to DS segment where buffer is
	inc	di
	dec	cx
	jnz	getb_read_loop		; Continue if more chars to read

getb_done:
	sti
	pop	ax			; AX = original length
	sub	ax, cx			; AX = chars read (original - remaining)

	pop	es
	pop	si
	pop	di
	pop	dx
	pop	cx
	pop	bx
	pop	bp
	ret
_port_getbuf	ENDP
