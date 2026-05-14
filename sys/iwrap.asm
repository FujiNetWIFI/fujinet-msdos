_TEXT	segment word public 'CODE'
	extern	intf5_:near
	extern	int28_work_:near
	extern	_fn_autoexec_armed:byte
	extern	_fn_old_int28:dword

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

	; INT 28h handler. Differs from the INTERRUPT macro because we must
	; chain to the previous handler via a far JMP (not IRET) so any other
	; idle-time TSRs keep working.
	PUBLIC	int28_vect_
int28_vect_ PROC FAR
	cmp	byte ptr cs:_fn_autoexec_armed, 0
	je	chain28

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

	call	int28_work_

	pop	es
	pop	ds
	pop	bp
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax

chain28:
	jmp	dword ptr cs:_fn_old_int28
int28_vect_ ENDP

_TEXT	ends

	end
