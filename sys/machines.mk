# Machine table, included by makefile once MACHINE has been defaulted.
#
# MACHINE names match the MAME driver short names, which is where the
# hardware facts came from - see machine.h and mach/<name>.inc.
#
#   TARGET     distributed 8.3 filename
#   MACHSYM_C  -D symbol for wcc  (empty for pc)
#   MACHSYM_A  -d symbol for wasm (empty for pc)
#   OBJP       object path prefix, with trailing '/'.
#              MUST be empty for pc and non-empty for every other machine:
#              that is what keeps the shared sys/print.obj a PC-only
#              artifact.  Eight other directories link it.
#   MACHOBJS   objects that only apply to this machine.  Spliced into OBJS
#              at the position the file used to occupy, so the link order -
#              and hence the image layout - is unchanged for the PC.

!ifeq MACHINE pc
TARGET     = fujinet.sys
MACHSYM_C  =
MACHSYM_A  =
OBJP       =
MACHOBJS   = $(OBJP)id8250.obj
!else ifeq MACHINE victor9k
TARGET     = fnvic9k.sys
MACHSYM_C  = -DMACH_VICTOR9K
MACHSYM_A  = -dMACH_VICTOR9K
OBJP       = obj/victor9k/
MACHOBJS   =
!else ifeq MACHINE apricot
TARGET     = fnapric.sys
MACHSYM_C  = -DMACH_APRICOT
MACHSYM_A  = -dMACH_APRICOT
OBJP       = obj/apricot/
MACHOBJS   =
!else ifeq MACHINE apricotf
TARGET     = fnaprf.sys
MACHSYM_C  = -DMACH_APRICOTF
MACHSYM_A  = -dMACH_APRICOTF
OBJP       = obj/apricotf/
MACHOBJS   =
!else ifeq MACHINE sanyo550
TARGET     = fnsanyo.sys
MACHSYM_C  = -DMACH_SANYO550
MACHSYM_A  = -dMACH_SANYO550
OBJP       = obj/sanyo550/
MACHOBJS   =
!else ifeq MACHINE tandy2k
TARGET     = fnt2000.sys
MACHSYM_C  = -DMACH_TANDY2K
MACHSYM_A  = -dMACH_TANDY2K
OBJP       = obj/tandy2k/
MACHOBJS   =
!else
!error Unknown MACHINE '$(MACHINE)' - try pc victor9k apricot apricotf sanyo550 tandy2k
!endif
