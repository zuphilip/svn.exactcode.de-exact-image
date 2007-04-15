
include build/top.make

BINARY = lib
BINARY_EXT = $(X_LIBEXT)
DEPS =

ifeq "$(WITHBARDECODE)" "1"
CPPFLAGS += $(BARDECODEINCS)
LDFLAGS += $(BARDECODELIBS)
else
NOT_SRCS += bardecode.cc
endif

CPPFLAGS += -I lib/

DEPS =

include build/bottom.make

