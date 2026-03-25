SYS = sys/fujinet.sys
NCOPY = ncopy/ncopy.exe
FNSHARE = fnshare/fnshare.exe
PRINTER = printer/fujiprn.sys
NGET = nget/nget.exe
NPUT = nput/nput.exe
FMOUNT = fmount/fmount.exe
FCONFIG = fconfig/fconfig.com

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
  $(wildcard $(dir $1)*.c $(dir $1)*.h $(dir $1)*.asm)
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

all: $(SYS) $(COMS) $(NCOPY) $(FNSHARE) $(PRINTER) $(NGET) $(NPUT) $(FMOUNT) $(FCONFIG)

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

# Create builds directory and copy all executables
builds: all
	@mkdir -p builds
	@echo -n "Copying executables to builds directory..."
	@cp -u $(SYS) $(PRINTER) $(NCOPY) $(FNSHARE) $(NGET) $(NPUT) $(FMOUNT) $(FCONFIG) config.sys builds/
	@echo "Done."

CLEAN_DIRS = $(sort $(dir $(SYS) $(COMS) $(NCOPY) $(FNSHARE) $(PRINTER) $(NGET) $(NPUT) $(FMOUNT) $(FCONFIG)))

clean:
	@echo "Cleaning up build artifacts..."
	@rm -rf builds
	@for d in $(CLEAN_DIRS); do rm -f $$d*.exe $$d*.obj $$d*.lib $$d*.com $$d*.sys; done
	@rm -f *.img
	@echo "Done."

sys/print.obj:
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

