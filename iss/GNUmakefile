WMAKE=wmake -h
DISK_IMG ?= iss.img

all:
	@$(WMAKE) -e CPPFLAGS=-DVERSION='\"$(shell git rev-parse --short HEAD)$(shell git status --porcelain | grep -q '^[ MADRCU]' && echo '*')\"'

disk: all
	@dd if=/dev/zero of=$(DISK_IMG) bs=1024 count=1440
	@mformat -i $(DISK_IMG)
	@mcopy -i $(DISK_IMG) iss.exe ::

clean:
	@$(WMAKE) clean
	@rm -f $(DISK_IMG)
