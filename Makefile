
X_BUILD_IMPLICIT=0
include utility/Makefile
X_BUILD_IMPLICIT=1

MODULES = lib bin
include $(addsuffix /Makefile,$(MODULES))

