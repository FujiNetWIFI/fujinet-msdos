SYS = sys/fujinet.sys
COMS = fujicom/fujicoms.lib
NCOPY = ncopy/ncopy.exe
FNSHARE = fnshare/fnshare.exe
PRINTER = printer/fujiprn.sys
NGET = nget/nget.exe

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

all: $(SYS) $(COMS) $(NCOPY) $(FNSHARE) $(PRINTER) $(NGET)

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
