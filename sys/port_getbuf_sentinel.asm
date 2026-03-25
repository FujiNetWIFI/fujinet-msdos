;-----------------------------------------------------------------------------
; uint16_t port_getbuf_sentinel(void *buf, uint16_t len, uint16_t timeout,
;				 uint8_t sentinel, uint16_t sentinel_count)
; Read multiple characters into buffer, stopping when:
;   - sentinel byte seen sentinel_count times, OR
;   - len bytes received, OR
;   - timeout expires
; Parameters: buf (pointer), len (word), timeout (word in ms),
;	      sentinel (byte), sentinel_count (word)
; Returns: Number of characters actually read in AX
;
; Register usage:
;   AX = scratch (UART data, calculations)
;   BX = BL = sentinel byte, BH = previous byte received
;   CX = remaining characters to read (counts down to 0)
;   DX = UART port addresses
;   DI = buffer pointer (auto-incremented)
;   SI = end tick count for current character timeout
;   BP = stack frame pointer
;   ES = segment 40h (BIOS data area for tick counter)
;
; Stack layout after all pushes:
;   [bp+12] = sentinel_count parameter
;   [bp+10] = sentinel parameter
;   [bp+8]  = timeout parameter
;   [bp+6]  = len parameter
;   [bp+4]  = buf parameter
;   [bp+2]  = return address
;   [bp+0]  = saved BP
;   [bp-2]  = saved BX
;   [bp-4]  = saved CX
;   [bp-6]  = saved DX
;   [bp-8]  = saved DI
;   [bp-10] = saved SI
;   [bp-12] = saved ES
;   [bp-14] = timeout in ticks
;   [bp-16] = sentinel_count (working copy)
;   [bp-18] = original length
;-----------------------------------------------------------------------------

; Stack offsets for port_getbuf_sentinel
ARG_BUF			EQU	[bp+4]
ARG_LEN			EQU	[bp+6]
ARG_TIMEOUT		EQU	[bp+8]
ARG_SENTINEL		EQU	[bp+10]
ARG_SENTCOUNT		EQU	[bp+12]

_port_getbuf_sentinel PROC NEAR
	push	bp			; [bp-0]
	mov	bp, sp
	push	bx			; [bp-2]
	push	cx			; [bp-4]
	push	dx			; [bp-6]
	push	di			; [bp-8]
	push	si			; [bp-10]
	push	es			; [bp-12]

	mov	di, ARG_BUF		; Get buffer pointer
	mov	cx, ARG_LEN		; CX = requested length (countdown)

	; Handle zero length case immediately
	test	cx, cx
	jz	getbs_done

	; Convert timeout from ms to ticks once (timeout / 55)
	mov	ax, ARG_TIMEOUT		; Get timeout parameter in ms
	xor	dx, dx
	push	cx			; [bp-16]
	mov	cx, 55
	div	cx			; AX = timeout in ticks
	pop	cx			; [bp-16]
	mov	ARG_TIMEOUT, ax		; save conterted timeout

	;; ; DEBUG: Print the timeout
	;; push	ax
	;; call	qemu_debug_char		; Print low byte
	;; mov	al, ah
	;; call	qemu_debug_char		; Print high byte
	;; ;; mov	ax, si
	;; ;; call	qemu_debug_char		; Print low byte
	;; ;; mov	al, ah
	;; ;; call	qemu_debug_char		; Print high byte
	;; pop	ax

	; Load sentinel byte into BL for fast access
	mov	bl, byte ptr ARG_SENTINEL ; BL = sentinel byte
	mov	bh, bl
	not	bh			; BH = inverted sentinel (can't match on first byte)

	; Set ES to BIOS data segment for tick counter access
	mov	ax, 40h
	mov	es, ax

getbs_read_loop:
	; Get start time for this character
	mov	si, es:[6Ch]		; SI = start tick count

	;; ; DEBUG: Print the timeout
	;; push	ax
	;; mov	ax, ARG_TIMEOUT_TICKS
	;; call	qemu_debug_char		; Print low byte
	;; mov	al, ah
	;; call	qemu_debug_char		; Print high byte
	;; mov	ax, si
	;; call	qemu_debug_char		; Print low byte
	;; mov	al, ah
	;; call	qemu_debug_char		; Print high byte
	;; pop	ax

	add	si, ARG_TIMEOUT

	;; ; DEBUG: Print the timeout
	;; push	ax
	;; ;; call	qemu_debug_char		; Print low byte
	;; ;; mov	al, ah
	;; ;; call	qemu_debug_char		; Print high byte
	;; mov	ax, si
	;; call	qemu_debug_char		; Print low byte
	;; mov	al, ah
	;; call	qemu_debug_char		; Print high byte
	;; pop	ax

	mov	dx, _port_uart_base
	add	dx, UART_LSR_OFF

getbs_wait_char:
	cli
	mov	ah, 32

getbs_skip_timeout:
	in	al, dx
	test	al, LSR_DR
	jnz	getbs_got_char

	dec	ah
	jnz	getbs_skip_timeout	; Don't need to check timeout constantly

	; Check if timeout expired
	sti
	mov	ax, es:[6Ch]		; Get current tick count

	;; ; DEBUG: Print the timeout
	;; push	ax
	;; call	qemu_debug_char		; Print low byte
	;; mov	al, ah
	;; call	qemu_debug_char		; Print high byte
	;; mov	ax, si
	;; call	qemu_debug_char		; Print low byte
	;; mov	al, ah
	;; call	qemu_debug_char		; Print high byte
	;; pop	ax

	cmp	ax, si			; Compare current to end time
	jb	getbs_wait_char		; Continue if not expired

	; Timeout - return what we have
	jmp	getbs_done

getbs_got_char:
	mov	dx, _port_uart_base
	add	dx, UART_RBR_OFF
	in	al, dx
	mov	ds:[di], al		; Write to DS segment where buffer is
	inc	di

	;; ; DEBUG: Print the character received
	;; push	ax
	;; call	qemu_debug_char		; Print the actual char
	;; pop	ax

	; Check if this is a sentinel
	cmp	al, bl
	jne	getbs_continue	; Not sentinel, just update prev and loop

	;; ; DEBUG: Print 'S' for sentinel detected
	;; push	ax
	;; mov	al, 'S'
	;; call	qemu_debug_char
	;; pop	ax

	; It's a sentinel - was previous (in BH) also sentinel?
	cmp	bh, bl			; Was previous char in bh also a sentinel?
	jne	getbs_count_sentinel	; No, count this sentinel

	;; ; DEBUG: Print 'N' for nullified sentinel
	;; push	ax
	;; mov	al, 'N'
	;; call	qemu_debug_char
	;; pop	ax

	; Double sentinel - nullify previous
	mov	byte ptr ds:[di-2], 0
	jmp	getbs_continue

getbs_count_sentinel:
	dec	word ptr ARG_SENTCOUNT	; Decrement sentinel counter on stack
	jz	getbs_sentinel_done

getbs_continue:
	mov	bh, al			; Update previous
	dec	cx
	jnz	getbs_read_loop		; Continue if more chars to read
	jmp	getbs_done

getbs_sentinel_done:
	; Found all sentinels - exit normally
	dec	cx			; Account for the last character

getbs_done:
	sti
	mov	ax, ARG_LEN
	sub	ax, cx			; AX = chars read (original - remaining)

	pop	es
	pop	si
	pop	di
	pop	dx
	pop	cx
	pop	bx
	pop	bp
	ret
_port_getbuf_sentinel ENDP
