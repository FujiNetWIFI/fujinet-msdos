WMAKE=wmake -h

all:
	@$(WMAKE) -e FUJINET_TRANSPORT="$(FUJINET_TRANSPORT)" CPPFLAGS=-DVERSION='\"$(shell git rev-parse --short HEAD)$(shell git status --porcelain | grep -q '^[ MADRCU]' && echo '*')\"'

clean:
	@$(WMAKE) FUJINET_TRANSPORT="$(FUJINET_TRANSPORT)" clean
