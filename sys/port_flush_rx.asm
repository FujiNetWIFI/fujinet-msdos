	PUBLIC	_port_flush_rx

;-----------------------------------------------------------------------------
; void port_flush_rx(void)
; Clear stale receive data before starting a new command transaction.
;-----------------------------------------------------------------------------
_port_flush_rx PROC NEAR
	push	dx
	push	ax

	; Clear 16550-compatible RX FIFO and keep FIFO enabled at 1-byte trigger.
	mov	dx, _port_uart_base
	add	dx, UART_FCR_OFF
	mov	al, FCR_ENABLE OR FCR_CLEAR_RX OR FCR_TRIGGER_1
	out	dx, al

	; Drain available bytes for 8250/16450 parts where FCR is ignored.
flush_rx_loop:
	mov	dx, _port_uart_base
	add	dx, UART_LSR_OFF
	in	al, dx
	test	al, LSR_DR
	jz	flush_rx_done
	mov	dx, _port_uart_base
	add	dx, UART_RBR_OFF
	in	al, dx
	jmp	flush_rx_loop

flush_rx_done:
	pop	ax
	pop	dx
	ret
_port_flush_rx ENDP
