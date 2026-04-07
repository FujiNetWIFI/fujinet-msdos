_TEXT	segment word public 'CODE'
	extern	int17_:near
	extern	idle_flush_:near
	extern	timer_tick_:near

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

	INTERRUPT	int17_

; Storage for old INT 08h handler address
	PUBLIC	_old_timer_off
	PUBLIC	_old_timer_seg
_old_timer_off	dw	0
_old_timer_seg	dw	0

; Storage for old INT 28h handler address
	PUBLIC	_old_idle_off
	PUBLIC	_old_idle_seg
_old_idle_off	dw	0
_old_idle_seg	dw	0

; INT 08h timer wrapper — sets DS=CS, calls timer_tick_, chains to old handler
	PUBLIC	timer_vect_
timer_vect_ PROC NEAR
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

	call	timer_tick_

	pop	es
	pop	ds
	pop	bp
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	jmp	dword ptr cs:[_old_timer_off]
timer_vect_ ENDP

; INT 28h idle wrapper — sets DS=CS, calls idle_flush_, chains to old handler
	PUBLIC	idle_vect_
idle_vect_ PROC NEAR
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

	call	idle_flush_

	pop	es
	pop	ds
	pop	bp
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	jmp	dword ptr cs:[_old_idle_off]
idle_vect_ ENDP

_TEXT	ends

	end
