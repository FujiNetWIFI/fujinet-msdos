TARGET  = cfgtsr.exe
CC      = wcc -q
CFLAGS  = -0 -bt=dos -ms -s -I../include -osh $(CPPFLAGS)
LD      = wlink OPTION quiet
LDFLAGS = SYSTEM dos OPTION MAP

all: $(TARGET)

$(TARGET): cfgtsr.obj

.obj.exe:
	$(LD) $(LDFLAGS) NAME $@ file {$<} LIBRARY {clibs.lib}
.c.obj: .AUTODEPEND
	$(CC) $(CFLAGS) -fo=$@ $<

clean : .SYMBOLIC
	rm -f $(TARGET) *.obj *.map *.err *.o
