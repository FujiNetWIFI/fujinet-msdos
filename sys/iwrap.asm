_TEXT	segment word public 'CODE'
	extern	intf5_:near
	extern	int21_inject_:near
	extern	_fn_autoexec_armed:byte
	extern	_fn_old_int21:dword

	; Macro to create an interrupt wrapper for a given C function
INTERRUPT MACRO func
	PUBLIC func&vect_
	func&vect_ PROC NEAR
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	bp
	push	ds
	push	es

; Set up data segment
	push	cs
	pop	ds

	call	func

	pop	es
	pop	ds
	pop	bp
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	iret
	func&vect_ ENDP
ENDM

	INTERRUPT	intf5_

	; INT 21h handler. While autorun is armed, watch every DOS call for an
	; input-style function. AH=0Ah with a large buffer is the COMMAND.COM
	; command line -- stuff the keyboard buffer and disarm. Any other
	; input call (DATE/TIME prompt, PAUSE, single-char read, etc.) means
	; we are NOT at a command prompt, so disarm and let the user drive.
	PUBLIC	int21_vect_
int21_vect_ PROC FAR
	cmp	byte ptr cs:_fn_autoexec_armed, 0
	je	i21_chain

	cmp	ah, 0Ah
	je	i21_check_0A
	cmp	ah, 0Ch
	je	i21_check_0C
	cmp	ah, 06h
	je	i21_check_06
	cmp	ah, 01h
	je	i21_abort
	cmp	ah, 07h
	je	i21_abort
	cmp	ah, 08h
	je	i21_abort
	jmp	i21_chain

i21_check_06:
	; AH=06h: DL=FFh means input, anything else is output.
	cmp	dl, 0FFh
	je	i21_abort
	jmp	i21_chain

i21_check_0C:
	; AH=0Ch: flush keyboard, then call input function in AL.
	cmp	al, 0Ah
	je	i21_check_0A
	cmp	al, 06h
	je	i21_check_06
	cmp	al, 01h
	je	i21_abort
	cmp	al, 07h
	je	i21_abort
	cmp	al, 08h
	je	i21_abort
	jmp	i21_chain

i21_check_0A:
	; DS:[DX] is caller's buffer; byte 0 is max size. Threshold 32 cleanly
	; separates the 128-byte command line from ~9-byte DATE/TIME prompts.
	push	bx
	mov	bx, dx
	cmp	byte ptr [bx], 32
	pop	bx
	jb	i21_abort

	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	bp
	push	ds
	push	es
	push	cs
	pop	ds
	call	int21_inject_
	pop	es
	pop	ds
	pop	bp
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	jmp	i21_chain

i21_abort:
	mov	byte ptr cs:_fn_autoexec_armed, 0
	jmp	i21_chain

i21_chain:
	jmp	dword ptr cs:_fn_old_int21
int21_vect_ ENDP

_TEXT	ends

	end
