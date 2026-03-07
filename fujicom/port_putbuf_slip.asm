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
	mov	dx, _port_uart_base
	add	dx, UART_LSR_OFF
	in	al, dx
	test	al, LSR_THRE
	jz	wait_loop

	pop	ax
	mov	dx, _port_uart_base
	add	dx, UART_THR_OFF
	out	dx, al
ENDM

;-----------------------------------------------------------------------------
; uint16_t port_putbuf_slip(void *buf, uint16_t len)
; Encode and transmit a SLIP-framed packet
;
; Operation:
; 1. Sends SLIP_END to mark frame start
; 2. Encodes and sends data:
;    - 0xC0 -> sends 0xDB 0xDC
;    - 0xDB -> sends 0xDB 0xDD
;    - Other -> sends as-is
; 3. Sends SLIP_END to mark frame end
;
; Parameters: buf (pointer), len (word)
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
;   [bp+6]  = len parameter
;   [bp+4]  = buf parameter
;   [bp+2]  = return address
;   [bp+0]  = saved BP
;   [bp-2]  = saved BX
;   [bp-4]  = saved CX
;   [bp-6]  = saved DX
;   [bp-8]  = saved SI
;   [bp-10] = saved DS
;-----------------------------------------------------------------------------

SLIP_PUT_PARAM_BUF	EQU	[bp+4]
SLIP_PUT_PARAM_LEN	EQU	[bp+6]

_port_putbuf_slip PROC NEAR
	push	bp			; [bp+0]
	mov	bp, sp
	push	bx			; [bp-2]
	push	cx			; [bp-4]
	push	dx			; [bp-6]
	push	si			; [bp-8]
	push	ds			; [bp-10]

	mov	si, SLIP_PUT_PARAM_BUF	; Get buffer pointer
	mov	cx, SLIP_PUT_PARAM_LEN	; CX = length to encode

	xor	bx, bx			; BX = encoded byte count

	; Send frame start marker
	mov	al, SLIP_END
	SLIP_SEND_BYTE
	inc	bx			; Count the frame marker

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
	; Send frame end marker
	mov	al, SLIP_END
	SLIP_SEND_BYTE
	inc	bx			; Count the frame marker

	mov	ax, bx			; Return encoded byte count

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
