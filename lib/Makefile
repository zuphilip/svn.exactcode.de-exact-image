include build/top.make

BINARY = lib
BINARY_EXT = $(X_LIBEXT)
DEPS =

# as fallback use the built-in Agg, we depend on it, ...

ifeq "$(WITHLIBAGG)" "0"
DEPS += $(agg-2.4_BINARY)
else
CPPFLAGS += $(LIBAGGINCS)
LDFLAGS += $(LIBAGGLIBS)
ifeq "$(WITHFREETYPE)" "1"
LDFLAGS += -laggfontfreetype
endif
endif

ifeq "$(WITHFREETYPE)" "1"
CPPFLAGS += $(FREETYPEINCS)
LDFLAGS += $(FREETYPELIBS) 
endif

CPPFLAGS += -I lib -I utility

include build/bottom.make
