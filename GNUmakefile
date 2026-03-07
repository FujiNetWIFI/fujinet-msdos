SYS = sys/fujinet.sys
COMS = fujicom/fujicoms.lib
NCOPY = ncopy/ncopy.exe
FNSHARE = fnshare/fnshare.exe
PRINTER = printer/fujiprn.sys
NGET = nget/nget.exe
ISS = iss/iss.exe

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
ISS_DEPS = $(call guess_deps,$(ISS))

$(info COMS_DEPS=$(COMS_DEPS))
$(info COMS=$(COMS))
$(info DIR=$(dir $(COMS)))

all: $(SYS) $(COMS) $(NCOPY) $(FNSHARE) $(PRINTER) $(NGET) $(ISS)

$(SYS): $(COMS) $(SYS_DEPS)
	$(build_it)

$(COMS): $(COMS_DEPS)
	$(build_it)
	rm $(SYS)

$(NCOPY): $(NCOPY_DEPS) sys/print.obj
	$(build_it)

$(FNSHARE): $(FNSHARE_DEPS)
	$(build_it)

$(PRINTER): $(PRINTER_DEPS)
	$(build_it)

$(NGET): $(NGET_DEPS) sys/print.obj
	$(build_it)

$(ISS): $(COMS) $(ISS_DEPS)
	$(build_it)

# Create builds directory and copy all executables
builds: all
	@mkdir -p builds
	@echo "Copying executables to builds directory..."
	@cp $(NCOPY) $(FNSHARE) $(NGET) $(ISS) builds/
	@echo "Done."

clean:
	@echo "Cleaning up build artifacts..."
	@rm -rf builds
	@rm -f sys/*.sys fujicom/*.lib ncopy/*.exe fnshare/*.exe printer/*.sys nget/*.exe iss/*.exe
	@rm -f sys/*.obj fujicom/*.obj ncopy/*.obj fnshare/*.obj printer/*.obj nget/*.obj iss/*.obj
	@echo "Done."

sys/print.obj:
	make -C $(dir $@)

zip: builds
	@echo "Creating fn-msdos.zip..."
	@zip -j fn-msdos.zip builds/*
	@echo "Done."  
