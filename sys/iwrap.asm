_TEXT	segment word public 'CODE'

	extern	intf5_:near

FUJIF5_DETECT_MAGIC	EQU	0F501h

	PUBLIC intf5_vect_
intf5_vect_ PROC NEAR
	cmp	ax, 0
	je	detect_

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

	call	intf5_

	pop	es
	pop	ds
	pop	bp
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	iret

detect_:
	push	cs
	pop	ds
	mov	si, OFFSET fujif5_signature_
	mov	ax, FUJIF5_DETECT_MAGIC
	push	bp
	mov	bp, sp
	and	WORD PTR [bp+6], 0FFFEh
	pop	bp
	iret
intf5_vect_ ENDP

fujif5_signature_	DB	"FUJINET", 0

_TEXT	ends

	end
