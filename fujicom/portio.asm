; 8250 UART Serial I/O Driver for 8088
; Implements serial communication functions for COM1 (port 0x3F8)
;
; Baud Rate Configuration (1.8432 MHz crystal):
;   115200 baud: divisor = 1
;    57600 baud: divisor = 2
;    38400 baud: divisor = 3
;    19200 baud: divisor = 6
;     9600 baud: divisor = 12
;     4800 baud: divisor = 24
;     2400 baud: divisor = 48
;
; Change BAUD_DIVISOR below to set desired baud rate

	;.model	small
	;.8086

	.data

; Global variable to store UART base address
_port_uart_base	DW	3F8h		; Default to COM1

	.code

	; Baud rate divisor - change this to set baud rate
BAUD_DIVISOR	EQU	1		; 115200 baud

	; 8250 UART Register Offsets (relative to base)
UART_RBR_OFF	EQU	0		; Receiver Buffer Register (read)
UART_THR_OFF	EQU	0		; Transmitter Holding Register (write)
UART_IER_OFF	EQU	1		; Interrupt Enable Register
UART_IIR_OFF	EQU	2		; Interrupt Identification Register
UART_LCR_OFF	EQU	3		; Line Control Register
UART_MCR_OFF	EQU	4		; Modem Control Register
UART_LSR_OFF	EQU	5		; Line Status Register
UART_MSR_OFF	EQU	6		; Modem Status Register
UART_DLL_OFF	EQU	0		; Divisor Latch Low (when DLAB=1)
UART_DLH_OFF	EQU	1		; Divisor Latch High (when DLAB=1)

	; Line Status Register bits
LSR_DR		EQU	01h		; Data Ready
LSR_THRE	EQU	20h		; Transmitter Holding Register Empty

	; Line Control Register bits
LCR_DLAB	EQU	80h		; Divisor Latch Access Bit
LCR_8N1		EQU	03h		; 8 data bits, no parity, 1 stop bit

	; Modem Control Register bits
MCR_DTR		EQU	01h		; Data Terminal Ready
MCR_RTS		EQU	02h		; Request To Send
MCR_OUT2	EQU	08h		; OUT2 (enables interrupts on PC)

	.code

	PUBLIC	_port_init
	PUBLIC	_port_getc
	PUBLIC	_port_getc_timeout
	PUBLIC	_port_getbuf
	PUBLIC	_port_getbuf_sentinel
	PUBLIC	_port_putc
	PUBLIC	_port_putbuf
	PUBLIC	_port_uart_base

; Debug helper - write character to QEMU debug port 0xE9
qemu_debug_char PROC	NEAR
	push	dx
	push	ax
	mov	dx, 0E9h
	out	dx, al
	pop	ax
	pop	dx
	ret
qemu_debug_char ENDP

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

;-----------------------------------------------------------------------------
; int port_getc(void)
; Read one character from the UART if available
; Returns: Character in AX (0-255), or -1 if no data available
;-----------------------------------------------------------------------------
_port_getc	PROC	NEAR
	push	dx

	mov	dx, _port_uart_base
	add	dx, UART_LSR_OFF
	in	al, dx
	test	al, LSR_DR		; Check if data ready
	jz	getc_no_data

	; Read the character
	mov	dx, _port_uart_base
	add	dx, UART_RBR_OFF
	in	al, dx
	xor	ah, ah			; Zero extend to word
	jmp	getc_done

getc_no_data:
	mov	ax, -1			; No data available

getc_done:
	pop	dx
	ret
_port_getc	ENDP

;-----------------------------------------------------------------------------
; int port_getc_timeout(uint16_t timeout)
; Wait for a character with timeout in milliseconds
; Parameters: timeout in stack (milliseconds)
; Returns: Character in AX (0-255), or -1 on timeout
;-----------------------------------------------------------------------------
_port_getc_timeout PROC NEAR
	push	bp
	mov	bp, sp
	push	bx
	push	cx
	push	dx
	push	es

	; Read starting BIOS tick count (0040:006C)
	mov	ax, 40h
	mov	es, ax
	mov	bx, es:[6Ch]		; Get current tick count

	; Convert timeout from ms to ticks (timeout / 55)
	mov	ax, [bp+4]		; Get timeout parameter in ms
	mov	cx, 55
	xor	dx, dx
	div	cx			; AX = timeout in ticks
	mov	cx, ax			; CX = timeout in ticks
	add	cx, bx			; CX = end tick count

getct_check:
	push	cx
	mov	dx, _port_uart_base
	add	dx, UART_LSR_OFF
	in	al, dx
	test	al, LSR_DR		; Check if data ready
	pop	cx
	jnz	getct_got_char

	; Check if timeout expired
	push	cx
	mov	ax, 40h
	mov	es, ax
	mov	ax, es:[6Ch]		; Get current tick count
	pop	cx
	cmp	ax, cx			; Compare current to end time
	jb	getct_check		; Continue if not expired

	; Timeout occurred
	mov	ax, -1
	jmp	getct_done

getct_got_char:
	; Read the character
	mov	dx, _port_uart_base
	add	dx, UART_RBR_OFF
	in	al, dx
	xor	ah, ah			; Zero extend to word

getct_done:
	pop	es
	pop	dx
	pop	cx
	pop	bx
	pop	bp
	ret
_port_getc_timeout ENDP

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
;   BX = timeout in ticks (constant)
;   CX = remaining characters to read (counts down to 0)
;   DX = UART port addresses
;   DI = buffer pointer (auto-incremented)
;   SI = end tick count for current character timeout
;   BP = stack frame pointer
;   ES = segment 40h (BIOS data area for tick counter)
;   Stack: [bp-2] = sentinel_count remaining
;	   [bp-4] = sentinel byte (extended to word)
;-----------------------------------------------------------------------------
_port_getbuf_sentinel PROC NEAR
	push	bp
	mov	bp, sp
	sub	sp, 4			; Allocate 4 bytes for local vars
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
	jz	getbs_done

	; Save sentinel parameters as local variables
	mov	al, [bp+10]		; Get sentinel byte
	xor	ah, ah
	mov	[bp-4], ax		; Save sentinel (extended to word)
	mov	ax, [bp+12]		; Get sentinel_count
	mov	[bp-2], ax		; Save sentinel_count remaining

	; Convert timeout from ms to ticks once (timeout / 55)
	mov	ax, [bp+8]		; Get timeout parameter in ms
	xor	dx, dx
	push	cx
	mov	cx, 55
	div	cx			; AX = timeout in ticks
	pop	cx
	push	ax			; Save timeout in ticks on stack

	push	cx			; Save original length on stack

	; Load sentinel byte into BL for fast access
	mov	bl, byte ptr [bp-4]	; BL = sentinel byte
	mov	bh, bl
	not	bh			; BH = inverted sentinel (can't match on first byte)

	; Set ES to BIOS data segment for tick counter access
	mov	ax, 40h
	mov	es, ax

getbs_read_loop:
	; Get start time for this character
	mov	si, es:[6Ch]		; SI = start tick count
	add	si, [bp-6]		; Add timeout ticks from stack
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
	cmp	ax, si			; Compare current to end time
	jb	getbs_wait_char		; Continue if not expired

	; Timeout - return what we have
	jmp	getbs_done

getbs_got_char:
	mov	dx, _port_uart_base
	add	dx, UART_RBR_OFF
	in	al, dx

	;; ; DEBUG: Print the character received
	;; push	ax
	;; call	qemu_debug_char		; Print the actual char
	;; pop	ax

	mov	ds:[di], al		; Write to DS segment where buffer is
	inc	di

	; Check if this is the sentinel byte
	cmp	al, bl			; Compare to sentinel in BL
	jne	getbs_continue		; Not sentinel, just continue

	;; ; DEBUG: Print 'S' for sentinel detected
	;; push	ax
	;; mov	al, 'S'
	;; call	qemu_debug_char
	;; pop	ax

	; It's a sentinel - check if previous (in BH) was also sentinel
	cmp	bh, bl			; Was previous also sentinel?
	jne	getbs_normal_sentinel	; No, count this one normally

	;; ; DEBUG: Print 'N' for nullified sentinel
	;; push	ax
	;; mov	al, 'N'
	;; call	qemu_debug_char
	;; pop	ax

	; Two sentinels in a row - zero out previous and skip decrement
	mov	byte ptr ds:[di-2], 0
	jmp	getbs_continue

getbs_normal_sentinel:
	; Normal sentinel - count it
	dec	word ptr [bp-2]
	jz	getbs_sentinel_done	; If count reached 0, we're done

getbs_continue:
	mov	bh, al			; Always save current char as previous
	dec	cx
	jnz	getbs_read_loop		; Continue if more chars to read
	jmp	getbs_done

getbs_sentinel_done:
	; Found all sentinels - exit normally
	dec	cx			; Account for the last character

getbs_done:
	sti
	pop	ax			; AX = original length
	sub	ax, cx			; AX = chars read (original - remaining)

	pop	es
	pop	si
	pop	di
	pop	dx
	pop	cx
	pop	bx
	mov	sp, bp			; Deallocate local variables
	pop	bp
	ret
_port_getbuf_sentinel ENDP

;-----------------------------------------------------------------------------
; int port_putc(uint8_t c)
; Send one character to the UART
; Parameters: c (byte) on stack
; Returns: Character sent in AX, or -1 on error
;-----------------------------------------------------------------------------
_port_putc	PROC	NEAR
	push	bp
	mov	bp, sp
	push	dx

putc_wait:
	mov	dx, _port_uart_base
	add	dx, UART_LSR_OFF
	in	al, dx
	test	al, LSR_THRE		; Check if transmitter ready
	jz	putc_wait

	; Send the character
	mov	al, [bp+4]		; Get character parameter
	mov	dx, _port_uart_base
	add	dx, UART_THR_OFF
	out	dx, al

	xor	ah, ah			; Return character in AX

	pop	dx
	pop	bp
	ret
_port_putc	ENDP

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

	END
