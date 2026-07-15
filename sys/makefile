TARGET  = fujinet.sys
AS      = wasm -q
ASFLAGS = -0 -mt -bt=DOS
CC      = wcc -q
CFLAGS  = -0 -bt=dos -ms -I../include -s -osh -zu $(CPPFLAGS) $(TRANSPORT_CPPFLAGS)
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

FUJINET_TRANSPORT =

!if "$(FUJINET_TRANSPORT)" == "NIO"
TRANSPORT_CPPFLAGS = -DFUJINET_TRANSPORT_NIO
TRANSPORT_ASFLAGS = -dFUJINET_TRANSPORT_NIO
TRANSPORT_CFILES = commands_nio.c nio.c nio_disk_protocol.c nio_protocol.c nio_transaction.c
F5_CFILES = intf5_nio.c
!else
TRANSPORT_CPPFLAGS =
TRANSPORT_ASFLAGS =
TRANSPORT_CFILES = commands.c
F5_CFILES = fujicom.c intf5.c
!endif

CFILES  = $(TRANSPORT_CFILES) dispatch.c port_config.c id8250.c init.c print.c setf5.c $(F5_CFILES)
AFILES  = header.asm portio.asm iwrap.asm
OBJS = $(CFILES:.c=.obj) $(AFILES:.asm=.obj)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) &
	  disable 1014, &
	  statics &
	  name $@ &
	  file {$(OBJS)} &
	  library {clibs.lib}

init.obj: init.c .AUTODEPEND
	$(CC) $(CFLAGS) -nt=_INIT -nc=INIT -fo=$@ $<

.c.obj: .AUTODEPEND
        $(CC) $(CFLAGS) -fo=$@ $< 
.asm.obj: .AUTODEPEND
	$(AS) $(ASFLAGS) $(TRANSPORT_ASFLAGS) -fo=$@ $<

clean : .SYMBOLIC
	rm -f $(TARGET) *.obj *.map *.err *.sys *.com *.exe
