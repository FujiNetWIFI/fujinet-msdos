# Build one FujiNet driver.  MACHINE selects which machine it is for;
# leaving it unset builds the IBM PC driver, fujinet.sys.
#
#   wmake -h                    -> fujinet.sys   (PC, objects flat in sys/)
#   wmake -h MACHINE=sanyo550   -> fnsanyo.sys   (objects in sys/obj/sanyo550/)
#
# Prefer GNU make (see GNUmakefile), which stamps VERSION from git and
# creates the object directories.

!ifndef MACHINE
MACHINE = pc
!endif
!include machines.mk

AS      = wasm -q
ASFLAGS = -0 -mt -bt=DOS $(MACHSYM_A)
CC      = wcc -q
CFLAGS  = -0 -bt=dos -ms -I../include -s -osh -zu $(MACHSYM_C) $(CPPFLAGS)
LD	= wlink OPTION quiet
LDFLAGS = &
	SYSTEM dos com &
	ORDER &
	  clname SYS_HEADER &
	  clname DATA &
	  clname CODE &
	  clname BSS &
	  clname INIT &
	OPTION MAP, NODEFAULTLIBS &
	LIBPATH ../fujicom

# Machine backends.  portio.asm includes exactly one of these; listing them
# all as dependencies is deliberate, so editing any backend rebuilds.
MACHINCS = slipdefs.inc mach/sio_regs.inc mach/pc.inc mach/sanyo550.inc &
           mach/tandy2k.inc mach/victor9k.inc mach/apricot.inc mach/apricotf.inc

# The SLIP framing bodies are textually included by portio.asm, so they are
# dependencies of portio.obj rather than translation units of their own.
PORTASM = port_getbuf_slip_dual.asm port_putc.asm port_putbuf_slip.asm

# Keep this order: it is the link order, and therefore the image layout.
OBJS = $(OBJP)commands.obj $(OBJP)dispatch.obj $(OBJP)fujicom.obj &
       $(MACHOBJS) &
       $(OBJP)init.obj $(OBJP)intf5.obj $(OBJP)print.obj &
       $(OBJP)header.obj $(OBJP)iwrap.obj $(OBJP)portio.obj

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) &
	  disable 1014, &
	  statics &
	  name $@ &
	  file {$(OBJS)} &
	  library {clibs.lib}

# Explicit rules throughout: wmake's extension rules derive the source name
# from the target name, so they cannot find ../print.c for a target in
# obj/<machine>/.
$(OBJP)init.obj: init.c .AUTODEPEND
	$(CC) $(CFLAGS) -nt=_INIT -nc=INIT -fo=$@ init.c

$(OBJP)commands.obj: commands.c .AUTODEPEND
	$(CC) $(CFLAGS) -fo=$@ commands.c
$(OBJP)dispatch.obj: dispatch.c .AUTODEPEND
	$(CC) $(CFLAGS) -fo=$@ dispatch.c
$(OBJP)fujicom.obj: fujicom.c .AUTODEPEND
	$(CC) $(CFLAGS) -fo=$@ fujicom.c
$(OBJP)id8250.obj: id8250.c .AUTODEPEND
	$(CC) $(CFLAGS) -fo=$@ id8250.c
$(OBJP)intf5.obj: intf5.c .AUTODEPEND
	$(CC) $(CFLAGS) -fo=$@ intf5.c
$(OBJP)print.obj: print.c .AUTODEPEND
	$(CC) $(CFLAGS) -fo=$@ print.c

$(OBJP)header.obj: header.asm
	$(AS) $(ASFLAGS) -fo=$@ header.asm
$(OBJP)iwrap.obj: iwrap.asm
	$(AS) $(ASFLAGS) -fo=$@ iwrap.asm
$(OBJP)portio.obj: portio.asm $(PORTASM) $(MACHINCS)
	$(AS) $(ASFLAGS) -fo=$@ portio.asm

# Only this machine's artifacts, so cleaning a variant cannot remove
# fujinet.sys or the shared sys/print.obj.  Use clobber for everything.
clean : .SYMBOLIC
	rm -f $(TARGET) $(OBJP)*.obj *.map *.err

clobber : .SYMBOLIC
	rm -rf obj
	rm -f *.obj *.map *.err *.sys *.com *.exe
