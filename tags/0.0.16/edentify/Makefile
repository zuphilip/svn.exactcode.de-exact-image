
include build/top.make

# build earch .cc file as executable - if this is not desired
# in the future a list must be supplied manually ...
BINARY = $(basename $(notdir $(wildcard $(X_MODULE)/*.cc)))

BINARY_EXT = $(X_EXEEXT)
DEPS = $(lib_BINARY) $(X_OUTARCH)/utility/ArgumentList$(X_OBJEXT) $(X_OUTARCH)/utility/File$(X_OBJEXT)

CPPFLAGS += -I utility

include build/bottom.make

