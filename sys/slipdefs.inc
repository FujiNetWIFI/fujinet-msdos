;-----------------------------------------------------------------------------
; SLIP framing constants (RFC 1055).
;
; Machine-independent: shared by every serial backend under mach/.
; The C side has its own copy of these in fujicom.c.
;-----------------------------------------------------------------------------

SLIP_END	EQU	0C0h
SLIP_ESC	EQU	0DBh
SLIP_ESC_END	EQU	0DCh
SLIP_ESC_ESC	EQU	0DDh
