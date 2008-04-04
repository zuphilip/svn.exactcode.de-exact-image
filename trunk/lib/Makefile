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

# as fallback use the built-in Agg, we depend on it, ...

ifeq "$(WITHLIBAGG)" "0"
DEPS += $(agg-2.4_BINARY)
else
CPPFLAGS += $(LIBAGGINCS)
LDFLAGS += $(LIBAGGLIBS)
ifeq "$(WITHFREETYPE)" "1"
CPPFLAGS += $(FREETYPEINCS)
LDFLAGS += $(FREETYPELIBS) -laggfontfreetype
endif
endif

CPPFLAGS += -I lib -I utility

include build/bottom.make
