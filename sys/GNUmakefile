WMAKE=wmake -h

# Non-PC machines.  The PC driver is the default target, built by `all`.
MACHINES=victor9k apricot apricotf sanyo550 tandy2k

# Stamp every binary in one run with the same version, so a `make machines`
# set is self-consistent.
GIT_REF := $(shell git rev-parse --short HEAD)$(shell git status --porcelain | grep -q '^[ MADRCU]' && echo '*')
VERFLAG = -e CPPFLAGS=-DVERSION='\"$(GIT_REF)\"'

all:
	@$(WMAKE) $(VERFLAG)

# Build the non-PC drivers.  Depends on `all` so the shared, PC-flavoured
# sys/print.obj is always the one left on disk for the eight other
# directories that link it.
machines: all $(MACHINES:%=machine-%)

machine-%:
	@mkdir -p obj/$*
	@$(WMAKE) $(VERFLAG) MACHINE=$*

clean:
	@$(WMAKE) clobber
