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

SYS_DEPS = $(guess_deps $(SYS))
COMS_DEPS = $(guess_deps $(COMS))
NCOPY_DEPS = $(guess_deps $(NCOPY))
FNSHARE_DEPS = $(guess_deps $(FNSHARE))
PRINTER_DEPS = $(guess_deps $(PRINTER))
NGET_DEPS = $(guess_deps $(NGET))
ISS_DEPS = $(guess_deps $(ISS))

all: $(SYS) $(COMS) $(NCOPY) $(FNSHARE) $(PRINTER) $(NGET) $(ISS)

$(SYS): $(COMS) $(SYS_DEPS)
	$(build_it)

$(COMS): $(COMS_DEPS)
	$(build_it)

$(NCOPY): $(NCOPY_DEPS)
	$(build_it)

$(FNSHARE): $(FNSHARE_DEPS)
	$(build_it)

$(PRINTER): $(PRINTER_DEPS)
	$(build_it)

$(NGET): $(NGET_DEPS)
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

zip: builds
	@echo "Creating fn-msdos.zip..."
	@zip -j fn-msdos.zip builds/*
	@echo "Done."  
