SYS = sys/fujinet.sys
NCOPY = ncopy/ncopy.exe
FNSHARE = fnshare/fnshare.exe
PRINTER = printer/fujiprn.sys
NGET = nget/nget.exe
NPUT = nput/nput.exe
FMOUNT = fmount/fmount.exe
FCONFIG = fconfig/fconfig.com
FMALL = fmall/fmall.com
FRESET = freset/freset.com
CFGTSR = cfgtsr/cfgtsr.exe

GIT_REF := $(shell git rev-parse --short HEAD)
ifdef USE_GIT_REF
DISK_IMG ?= fn-$(GIT_REF).img
else
DISK_IMG ?= fn-msdos.img
endif

define build_it
	make -C $(dir $@)
endef

define guess_deps
  $(wildcard $(dir $1)*.c $(dir $1)*.h $(dir $1)*.asm $(dir $1)*.inc)
endef

SYS_DEPS = $(call guess_deps,$(SYS))
COMS_DEPS = $(call guess_deps,$(COMS))
NCOPY_DEPS = $(call guess_deps,$(NCOPY))
FNSHARE_DEPS = $(call guess_deps,$(FNSHARE))
PRINTER_DEPS = $(call guess_deps,$(PRINTER))
NGET_DEPS = $(call guess_deps,$(NGET))
NPUT_DEPS = $(call guess_deps,$(NPUT))
FMOUNT_DEPS = $(call guess_deps,$(FMOUNT))
FCONFIG_DEPS = $(call guess_deps,$(FCONFIG))
FMALL_DEPS = $(call guess_deps,$(FMALL))
FRESET_DEPS = $(call guess_deps,$(FRESET))
CFGTSR_DEPS = $(call guess_deps,$(CFGTSR))

# Drivers for non-IBM-PC MS-DOS machines.  Not part of `all`: a PC user has
# no use for them, and `builds`/`zip`/`disk` stay PC-only for the same
# reason.  Use `make machines` or `make builds-machines`.
MACHINES_SYS = sys/fnvic9k.sys sys/fnapric.sys sys/fnaprf.sys \
               sys/fnsanyo.sys sys/fnt2000.sys

all: $(SYS) $(COMS) $(NCOPY) $(FNSHARE) $(PRINTER) $(NGET) $(NPUT) $(FMOUNT) $(FCONFIG) $(FMALL) $(FRESET) $(CFGTSR)

# $(SYS) first: it leaves the shared, PC-flavoured sys/print.obj in place.
machines: $(SYS) $(SYS_DEPS)
	make -C sys machines

$(MACHINES_SYS): machines

$(SYS): $(COMS) $(SYS_DEPS)
	$(build_it)

$(COMS): $(COMS_DEPS)
	$(build_it)
	rm $(SYS) || true

$(NCOPY): $(NCOPY_DEPS) sys/print.obj
	$(build_it)

$(FNSHARE): $(FNSHARE_DEPS)
	$(build_it)

$(PRINTER): $(PRINTER_DEPS)
	$(build_it)

$(NGET): $(NGET_DEPS) sys/print.obj
	$(build_it)

$(NPUT): $(COMS) $(NPUT_DEPS) sys/print.obj
	$(build_it)

$(FMOUNT): $(COMS) $(FMOUNT_DEPS)
	$(build_it)

$(FCONFIG): $(FCONFIG_DEPS)
	cd $(dir $@) && sh build.sh

$(FMALL): $(FMALL_DEPS)
	$(build_it)

$(FRESET): $(FRESET_DEPS)
	$(build_it)

$(CFGTSR): $(CFGTSR_DEPS)
	$(build_it)

# Create builds directory and copy all executables
builds: all
	@mkdir -p builds
	@echo -n "Copying executables to builds directory..."
	@cp -u $(SYS) $(PRINTER) $(NCOPY) $(FNSHARE) $(NGET) $(NPUT) $(FMOUNT) $(FCONFIG) $(FMALL) $(FRESET) $(CFGTSR) config.sys builds/
	@echo "Done."

# Collect the non-PC drivers for distribution.
builds-machines: machines
	@mkdir -p builds
	@echo -n "Copying machine drivers to builds directory..."
	@cp -u $(MACHINES_SYS) builds/
	@echo "Done."

CLEAN_DIRS = $(sort $(dir $(SYS) $(COMS) $(NCOPY) $(FNSHARE) $(PRINTER) $(NGET) $(NPUT) $(FMOUNT) $(FCONFIG) $(FMALL) $(FRESET) $(CFGTSR)))

clean:
	@echo "Cleaning up build artifacts..."
	@rm -rf builds sys/obj
	@for d in $(CLEAN_DIRS); do rm -f $$d*.exe $$d*.obj $$d*.lib $$d*.com $$d*.sys; done
	@rm -f *.img
	@echo "Done."

# Shared by printer/ ncopy/ nget/ nput/ fmount/ fnshare/ iss/ and nc/.
# It must always be the PC build of print.c (INT 10h console output); the
# non-PC drivers keep theirs in sys/obj/<machine>/.  Listing prerequisites
# matters: without them make treats an existing print.obj as up to date
# forever and never regenerates a stale one.
sys/print.obj: sys/print.c sys/print.h sys/machine.h
	make -C $(dir $@)

zip: builds
	@echo "Creating fn-msdos.zip..."
	@zip -j fn-msdos.zip builds/*
	@echo "Done."

disk: builds
	@echo "Creating 1.44MB floppy..."
	@dd if=/dev/zero of=$(DISK_IMG) bs=1024 count=1440
	@mformat -i $(DISK_IMG)
	@mcopy -i $(DISK_IMG) builds/* ::
	@echo "Created disk image $(DISK_IMG). Done."

