	PUBLIC	_port_getbuf_slip_dual

;-----------------------------------------------------------------------------
; Macro to wait for a character with timeout
; On entry: SI = end tick count, ES = BIOS_DATA_SEG
; On exit: AL = character received, or jumps to timeout_label if timeout
; Destroys: AH, DX
;-----------------------------------------------------------------------------
SLIPD_WAIT_CHAR MACRO timeout_label
	LOCAL wait_loop, skip_timeout, got_char

	mov	dx, SLIPD_LOCAL_UART_BASE
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
	mov	ax, es:[BIOS_TICK_OFFSET]
	cmp	ax, si
	jb	wait_loop

	; Timeout occurred
	jmp	timeout_label

got_char:
	mov	dx, SLIPD_LOCAL_UART_BASE
	add	dx, UART_RBR_OFF
	in	al, dx
ENDM

;-----------------------------------------------------------------------------
; uint16_t port_getbuf_slip_dual(void *hdr_buf, uint16_t hdr_len,
;				  void far *data_buf, uint16_t data_len,
;				  uint16_t timeout)
; Read and decode a SLIP-framed packet into two buffers
;
; Operation: Same as port_getbuf_slip but splits output:
; - First hdr_len bytes go to hdr_buf (near pointer, DS segment)
; - Remaining bytes go to data_buf (far pointer, specified segment)
;
; Parameters:
;   hdr_buf (near pointer), hdr_len (word),
;   data_buf (far pointer - segment:offset), data_len (word),
;   timeout (word in ms)
; Returns: Total number of decoded bytes (header + data)
;
; Register usage:
;   AX = scratch
;   BX = total bytes written (accumulator)
;   CX = remaining space in current buffer
;   DX = UART port / scratch
;   DI = buffer pointer
;   SI = end tick count for timeout
;   BP = stack frame
;   DS = current buffer segment (header, then data after switch)
;   ES = BIOS_DATA_SEG (0x40) for tick counter
;
; Stack layout:
;   [bp+14] = timeout parameter (converted to ticks in place)
;   [bp+12] = data_len parameter
;   [bp+10] = data_buf offset
;   [bp+8]  = data_buf segment
;   [bp+6]  = hdr_len parameter
;   [bp+4]  = hdr_buf parameter
;   [bp+2]  = return address
;   [bp+0]  = saved BP
;   [bp-2]  = saved BX
;   [bp-4]  = saved CX
;   [bp-6]  = saved DX
;   [bp-8]  = saved DI
;   [bp-10] = saved SI
;   [bp-12] = saved ES
;   [bp-14] = saved DS (original)
;   [bp-16] = _port_uart_base (copy)
;-----------------------------------------------------------------------------

SLIPD_PARAM_HDR_BUF	EQU	[bp+4]
SLIPD_PARAM_HDR_LEN	EQU	[bp+6]
SLIPD_PARAM_DATA_OFF	EQU	[bp+8]
SLIPD_PARAM_DATA_SEG	EQU	[bp+10]
SLIPD_PARAM_DATA_LEN	EQU	[bp+12]
SLIPD_PARAM_TIMEOUT	EQU	[bp+14]
SLIPD_LOCAL_UART_BASE	EQU	[bp-16]

_port_getbuf_slip_dual PROC NEAR
	push	bp			; [bp+0]
	mov	bp, sp
	push	bx			; [bp-2]
	push	cx			; [bp-4]
	push	dx			; [bp-6]
	push	di			; [bp-8]
	push	si			; [bp-10]
	push	es			; [bp-12]

	push	ds			; [bp-14] Save original DS

	mov	di, SLIPD_PARAM_HDR_BUF	; Start with header buffer
	mov	cx, SLIPD_PARAM_HDR_LEN	; CX = header length (remaining)

	; Save _port_uart_base on stack for macro to use
	mov	ax, _port_uart_base
	push	ax			; [bp-16]

	; Check for zero total length
	mov	ax, SLIPD_PARAM_HDR_LEN
	add	ax, SLIPD_PARAM_DATA_LEN
	test	ax, ax
	jz	slipd_done		; Zero total length

	; Convert timeout from ms to ticks and store in place
	mov	ax, SLIPD_PARAM_TIMEOUT
	xor	dx, dx
	push	cx
	mov	cx, 55
	div	cx			; AX = timeout in ticks
	pop	cx
	mov	SLIPD_PARAM_TIMEOUT, ax	; Overwrite parameter with ticks

	; BX = bytes written accumulator
	xor	bx, bx

	; Set ES to BIOS data segment
	mov	ax, BIOS_DATA_SEG
	mov	es, ax

	; Phase 1: Sync to frame - discard until SLIP_END
slipd_sync:
	mov	si, es:[BIOS_TICK_OFFSET]
	add	si, SLIPD_PARAM_TIMEOUT
	SLIPD_WAIT_CHAR slipd_done
	cmp	al, SLIP_END
	jne	slipd_sync

	; Phase 2: Skip additional SLIP_END bytes
slipd_skip_end:
	mov	si, es:[BIOS_TICK_OFFSET]
	add	si, SLIPD_PARAM_TIMEOUT
	SLIPD_WAIT_CHAR slipd_done
	cmp	al, SLIP_END
	je	slipd_skip_end

	; Phase 3: Decode - AL has first data byte
slipd_decode_loop:
	cmp	al, SLIP_END
	je	slipd_done

	cmp	al, SLIP_ESC
	je	slipd_handle_escape

slipd_store_byte:
	; Write byte to current buffer (DS:[DI])
	mov	ds:[di], al
	inc	di
	inc	bx			; Count byte written
	dec	cx
	jnz	slipd_read_next		; Still bytes remaining in current buffer

	; Current buffer exhausted - check if we need to switch to data buffer
	mov	cx, SLIPD_PARAM_DATA_LEN
	test	cx, cx
	jz	slipd_done		; No data buffer, we're done

	; Zero out data_len so exhausting the data buffer ends the loop
	mov	word ptr SLIPD_PARAM_DATA_LEN, 0

	; Switch to data buffer
	mov	di, SLIPD_PARAM_DATA_OFF
	mov	dx, SLIPD_PARAM_DATA_SEG
	mov	ds, dx			; Switch DS to data segment
	; Fall through to slipd_read_next

slipd_read_next:
	; Read next byte
	mov	si, es:[BIOS_TICK_OFFSET]
	add	si, SLIPD_PARAM_TIMEOUT
	SLIPD_WAIT_CHAR slipd_done
	jmp	slipd_decode_loop

slipd_handle_escape:
	; Read escaped byte
	mov	si, es:[BIOS_TICK_OFFSET]
	add	si, SLIPD_PARAM_TIMEOUT
	SLIPD_WAIT_CHAR slipd_done

	; Decode escape sequence
	cmp	al, SLIP_ESC_END
	jne	slipd_check_esc_esc
	mov	al, SLIP_END
	jmp	slipd_store_byte

slipd_check_esc_esc:
	cmp	al, SLIP_ESC_ESC
	jne	slipd_store_byte	; Invalid - store as-is
	mov	al, SLIP_ESC
	jmp	slipd_store_byte

slipd_done:
	sti
	mov	ax, bx			; AX = total bytes written

	pop	dx			; [bp-16] Discard _port_uart_base copy

	pop	ds			; [bp-14]
	pop	es			; [bp-12]
	pop	si			; [bp-10]
	pop	di			; [bp-8]
	pop	dx			; [bp-6]
	pop	cx			; [bp-4]
	pop	bx			; [bp-2]
	pop	bp			; [bp+0]
	ret

_port_getbuf_slip_dual ENDP
