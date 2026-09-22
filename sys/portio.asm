; FujiNet serial transport for MS-DOS
;
; SLIP framing over a machine-specific serial port.  The framing and the
; DOS-facing entry points are portable; everything that touches hardware
; lives in a backend under mach/, selected at build time by the makefile.
;
; To add a machine: write mach/<name>.inc against the contract documented at
; the top of mach/pc.inc, add a case to the IFDEF chain below, and add the
; machine to the MACHINES list in the makefile.  See also machine.h, which
; carries the matching C-side constants.

	;.model	small
	;.8086

	PUBLIC	_port_uart_base

	include slipdefs.inc

;-----------------------------------------------------------------------------
; Machine backend selection.  Keep in step with MACH_* in machine.h.
;-----------------------------------------------------------------------------

IFDEF MACH_VICTOR9K
	include mach/victor9k.inc
ELSEIFDEF MACH_APRICOT
	include mach/apricot.inc
ELSEIFDEF MACH_APRICOTF
	include mach/apricotf.inc
ELSEIFDEF MACH_SANYO550
	include mach/sanyo550.inc
ELSEIFDEF MACH_TANDY2K
	include mach/tandy2k.inc
ELSE
	include mach/pc.inc		; IBM PC and compatibles (default)
ENDIF

; A backend may claim ES for register addressing or for its timebase, but
; not for both - the receive path only has the one spare segment register.
IFDEF PORT_NEEDS_ES_FOR_HW
IFDEF PORT_NEEDS_ES_FOR_TIME
	.err <backend claims ES for both register access and timebase>
ENDIF
ENDIF

;-----------------------------------------------------------------------------
; Machine-independent SLIP framing.
;-----------------------------------------------------------------------------

	include port_getbuf_slip_dual.asm
	include port_putc.asm
	include port_putbuf_slip.asm

	END
