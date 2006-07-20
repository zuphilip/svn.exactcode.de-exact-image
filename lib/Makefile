
ifeq "$(WITHJASPER)" "1"
LDFLAGS += -ljasper
else
NOT_SRCS += jpeg2000.cc
endif

ifeq "$(WITHLIBUNGIF)" "1"
LDFLAGS += -llungif
else
NOT_SRCS += gif.cc
endif

ifeq "$(WITHLCMS)" "1"
LDFLAGS += -llcms
endif

include build/top.make

BINARY = lib
BINARY_EXT = $(X_LIBEXT)
DEPS =

CPPFLAGS += -I lib/
LDFLAGS += -ljpeg -ltiff -lpng -lnetpbm

DEPS = $(X_OUTARCH)/libbmp/bmp$(X_OBJEXT)

include build/bottom.make

