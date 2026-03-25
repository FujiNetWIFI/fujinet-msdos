	PUBLIC	_port_putbuf_slip

;-----------------------------------------------------------------------------
; Macro to wait for transmitter ready and send a byte
; On entry: AL = byte to send
; Destroys: DX
;-----------------------------------------------------------------------------
SLIP_SEND_BYTE MACRO
	LOCAL wait_loop

	push	ax
wait_loop:
	mov	dx, SLIP_PUT_LOCAL_UART_BASE
	add	dx, UART_LSR_OFF
	in	al, dx
	test	al, LSR_THRE
	jz	wait_loop

	pop	ax
	mov	dx, SLIP_PUT_LOCAL_UART_BASE
	add	dx, UART_THR_OFF
	out	dx, al
ENDM

;-----------------------------------------------------------------------------
; uint16_t port_putbuf_slip(const void far *buf, uint16_t len)
; Encode and transmit a SLIP-framed packet
;
; Operation:
; Encodes and sends data:
;   - 0xC0 -> sends 0xDB 0xDC
;   - 0xDB -> sends 0xDB 0xDD
;   - Other -> sends as-is
;
; Parameters: buf (far pointer), len (word)
; Returns: Number of encoded bytes transmitted (including frame markers)
;
; Register usage:
;   AL = current byte being processed
;   BX = encoded byte count
;   CX = remaining bytes to encode (counts down to 0)
;   DX = UART port addresses
;   SI = source buffer pointer (auto-incremented)
;   BP = stack frame pointer
;
; Stack layout:
;   [bp+8]  = len parameter
;   [bp+6]  = buf segment
;   [bp+4]  = buf offset
;   [bp+2]  = return address
;   [bp+0]  = saved BP
;   [bp-2]  = saved BX
;   [bp-4]  = saved CX
;   [bp-6]  = saved DX
;   [bp-8]  = saved SI
;   [bp-10] = saved DS
;-----------------------------------------------------------------------------

SLIP_PUT_PARAM_BUF_OFF	EQU	[bp+4]
SLIP_PUT_PARAM_BUF_SEG	EQU	[bp+6]
SLIP_PUT_PARAM_LEN	EQU	[bp+8]
SLIP_PUT_LOCAL_UART_BASE EQU	[bp-12]

_port_putbuf_slip PROC NEAR
	push	bp			; [bp+0]
	mov	bp, sp
	push	bx			; [bp-2]
	push	cx			; [bp-4]
	push	dx			; [bp-6]
	push	si			; [bp-8]
	push	ds			; [bp-10]

	; Save _port_uart_base on stack before switching DS
	mov	ax, _port_uart_base
	push	ax				; [bp-12]

	mov	si, SLIP_PUT_PARAM_BUF_OFF	; Get buffer offset
	mov	ax, SLIP_PUT_PARAM_BUF_SEG	; Get buffer segment
	mov	ds, ax				; Switch DS to buffer segment
	mov	cx, SLIP_PUT_PARAM_LEN		; CX = length to encode

	xor	bx, bx			; BX = encoded byte count

	; Encode and send data
	test	cx, cx
	jz	slip_put_end		; Nothing to send

slip_put_loop:
	lodsb				; Load byte from [DS:SI], increment SI

	cmp	al, SLIP_END
	je	slip_put_encode_end
	cmp	al, SLIP_ESC
	je	slip_put_encode_esc

slip_put_send:
	; Send the byte in AL (either regular or final escape byte)
	SLIP_SEND_BYTE
	inc	bx			; Count it
	dec	cx
	jnz	slip_put_loop

slip_put_end:
	mov	ax, bx			; Return encoded byte count

	pop	dx			; [bp-12] Discard _port_uart_base copy
	pop	ds			; [bp-10]
	pop	si			; [bp-8]
	pop	dx			; [bp-6]
	pop	cx			; [bp-4]
	pop	bx			; [bp-2]
	pop	bp			; [bp+0]
	ret

slip_put_encode_end:
	; Encode 0xC0 as 0xDB 0xDC
	mov	al, SLIP_ESC
	SLIP_SEND_BYTE
	inc	bx
	mov	al, SLIP_ESC_END
	jmp	slip_put_send

slip_put_encode_esc:
	; Encode 0xDB as 0xDB 0xDD
	mov	al, SLIP_ESC
	SLIP_SEND_BYTE
	inc	bx
	mov	al, SLIP_ESC_ESC
	jmp	slip_put_send

_port_putbuf_slip ENDP
