_TEXT	segment word public 'CODE'

	extern	intf5_:near

FUJIF5_DETECT_MAGIC	EQU	0F501h

	PUBLIC intf5_vect_
intf5_vect_ PROC NEAR
IFDEF FUJINET_TRANSPORT_NIO
	cmp	ax, 0
	je	detect_
ENDIF

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

IFDEF FUJINET_TRANSPORT_NIO
detect_:
	push	ds
	push	cs
	pop	es
	mov	si, OFFSET fujif5_signature_
	mov	ax, FUJIF5_DETECT_MAGIC
	push	bp
	mov	bp, sp
	and	WORD PTR [bp+8], 0FFFEh
	pop	bp
	pop	ds
	iret
ENDIF
intf5_vect_ ENDP

IFDEF FUJINET_TRANSPORT_NIO
fujif5_signature_	DB	"FUJINET", 0
ENDIF

_TEXT	ends

	end
