;-----------------------------------------------------------------------------
; Macro to wait for a character with timeout
; On entry: SI = end tick count, ES = segment 40h
; On exit: AL = character received, or jumps to timeout_label if timeout
; Destroys: AH, DX
;-----------------------------------------------------------------------------
SLIP_WAIT_CHAR MACRO timeout_label
	LOCAL wait_loop, skip_timeout, got_char

	mov	dx, _port_uart_base
	add	dx, UART_LSR_OFF

wait_loop:
	cli
	mov	ah, 32

skip_timeout:
	in	al, dx
	test	al, LSR_DR
	jnz	got_char

	dec	ah
	jnz	skip_timeout

	sti
	mov	ax, es:[6Ch]
	cmp	ax, si
	jb	wait_loop

	; Timeout occurred
	jmp	timeout_label

got_char:
	mov	dx, _port_uart_base
	add	dx, UART_RBR_OFF
	in	al, dx

ENDM

;-----------------------------------------------------------------------------
; uint16_t port_getbuf_slip(void *buf, uint16_t len, uint16_t timeout)
; Read and decode a SLIP-framed packet
;
; Operation:
; 1. Discards bytes until SLIP_END (0xC0) is found
; 2. Skips consecutive SLIP_END bytes
; 3. Decodes SLIP escape sequences:
;    - 0xDB 0xDC -> 0xC0
;    - 0xDB 0xDD -> 0xDB
; 4. Stops on: SLIP_END, buffer full, or timeout
;
; Parameters: buf (pointer), len (word), timeout (word in ms)
; Returns: Number of decoded bytes in buffer
;
; Register usage:
;   AX = scratch (UART data, calculations, tick counter)
;   BX = timeout in ticks (constant after initialization)
;   CX = remaining buffer space (counts down to 0)
;   DX = UART port addresses
;   DI = buffer pointer (auto-incremented as bytes are written)
;   SI = end tick count for current character timeout
;   BP = stack frame pointer
;   ES = segment 40h (BIOS data area for tick counter)
;
; Stack layout:
;   [bp+8]  = timeout parameter
;   [bp+6]  = len parameter
;   [bp+4]  = buf parameter
;   [bp-2]  = saved BX
;   [bp-4]  = saved CX
;   [bp-6]  = saved DX
;   [bp-8]  = saved DI
;   [bp-10] = saved SI
;   [bp-12] = saved ES
;   [bp-14] = timeout in ticks
;   [bp-16] = original length
;-----------------------------------------------------------------------------

SLIP_PARAM_BUF		EQU	[bp+4]
SLIP_PARAM_LEN		EQU	[bp+6]
SLIP_PARAM_TIMEOUT	EQU	[bp+8]
SLIP_LOCAL_TIMEOUT_TICKS EQU	[bp-14]
SLIP_LOCAL_ORIGLEN	EQU	[bp-16]

_port_getbuf_slip PROC NEAR
	push	bp			; [bp-0]
	mov	bp, sp
	push	bx			; [bp-2]
	push	cx			; [bp-4]
	push	dx			; [bp-6]
	push	di			; [bp-8]
	push	si			; [bp-10]
	push	es			; [bp-12]

	mov	di, SLIP_PARAM_BUF	; Get buffer pointer
	mov	cx, SLIP_PARAM_LEN	; CX = buffer length

	; Convert timeout from ms to ticks once (timeout / 55)
	mov	ax, SLIP_PARAM_TIMEOUT
	xor	dx, dx
	push	cx
	mov	cx, 55
	div	cx			; AX = timeout in ticks
	pop	cx
	mov	bx, ax			; BX = timeout in ticks
	push	bx			; [bp-14] Save timeout ticks
	push	cx			; [bp-16] Save original length

	; Handle zero length case
	test	cx, cx
	jz	slip_done

	; Set ES to BIOS data segment for tick counter access
	mov	ax, 40h
	mov	es, ax

	; Phase 1: Sync to frame - discard until we see SLIP_END
slip_sync:
	mov	si, es:[6Ch]
	add	si, SLIP_LOCAL_TIMEOUT_TICKS
	SLIP_WAIT_CHAR slip_done
	cmp	al, SLIP_END
	jne	slip_sync		 ; Keep looking for frame start

	; Phase 2: Skip any additional SLIP_END bytes
slip_skip_end:
	mov	si, es:[6Ch]
	add	si, SLIP_LOCAL_TIMEOUT_TICKS
	SLIP_WAIT_CHAR slip_done
	cmp	al, SLIP_END
	je	slip_skip_end		 ; Another END, keep skipping

	; Phase 3: Decode the frame
	; AL has first non-END byte, fall through to main decode loop

slip_decode_loop:
	; Check what type of byte we have in AL
	cmp	al, SLIP_END
	je	slip_done		 ; End of frame

	cmp	al, SLIP_ESC
	je	slip_handle_escape

slip_store_byte:
	; Regular byte (or decoded escape) - store it
	mov	ds:[di], al
	inc	di
	dec	cx
	jz	slip_done		 ; Buffer full

	; Read next byte and continue
	mov	si, es:[6Ch]
	add	si, SLIP_LOCAL_TIMEOUT_TICKS
	SLIP_WAIT_CHAR slip_done
	jmp	slip_decode_loop

slip_handle_escape:
	; Read the next byte after ESC to determine what to write
	mov	si, es:[6Ch]
	add	si, SLIP_LOCAL_TIMEOUT_TICKS
	SLIP_WAIT_CHAR slip_done

	; Decode the escape sequence
	cmp	al, SLIP_ESC_END
	jne	slip_check_esc_esc
	mov	al, SLIP_END		 ; ESC + ESC_END -> write END
	jmp	slip_store_byte

slip_check_esc_esc:
	cmp	al, SLIP_ESC_ESC
	jne	slip_store_byte		 ; Invalid escape - write as-is
	mov	al, SLIP_ESC		 ; ESC + ESC_ESC -> write ESC
	jmp	slip_store_byte

slip_done:
	sti
	pop	ax			; [bp-16] AX = original length
	sub	ax, cx			; AX = bytes decoded

	pop	bx			; [bp-14] discard timeout ticks
	pop	es			; [bp-12]
	pop	si			; [bp-10]
	pop	di			; [bp-8]
	pop	dx			; [bp-6]
	pop	cx			; [bp-4]
	pop	bx			; [bp-2]
	pop	bp			; [bp-0]
	ret
_port_getbuf_slip ENDP
