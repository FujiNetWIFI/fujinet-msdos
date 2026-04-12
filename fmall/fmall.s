        ;;
        ;; Quickly Mount All FujiNet slots
        ;;

        cpu     8086
        org     100h

_start:
        mov     dx,0000H	; No payload
        mov     ax,0D770H       ; Device $70, Command D7 (MOUNT ALL)
        mov     cx,0000H        ; No Aux1/2
        mov     si,0000H        ; No Aux3/4
        int     0F5H            ; Call FujiNet

	mov	DX,MSG		; Set to success message, unless...
	cmp	AL,'E'          ; Error?
	jne	OUT		; Nope, jump over error msg
	mov	DX,ERR		; Error happened, point to errmsg. 
        mov     AH,09H          ; Call display string
OUT:    int     21H             ; Call DOS

        INT     20H             ; Exit back to DOS

MSG     db      'All Device Slots Mounted.', 0dH, 0aH, '$'
ERR	db	'Mount Error', 0dH, 0aH, '$'
