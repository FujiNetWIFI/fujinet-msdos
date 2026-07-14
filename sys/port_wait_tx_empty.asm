	PUBLIC	_port_wait_tx_empty

;-----------------------------------------------------------------------------
; void port_wait_tx_empty(void)
; Wait until the final transmitted byte has left the UART.
;-----------------------------------------------------------------------------
_port_wait_tx_empty PROC NEAR
	push	dx
	push	ax

wait_tx_empty:
	mov	dx, _port_uart_base
	add	dx, UART_LSR_OFF
	in	al, dx
	test	al, LSR_TEMT
	jz	wait_tx_empty

	pop	ax
	pop	dx
	ret
_port_wait_tx_empty ENDP
