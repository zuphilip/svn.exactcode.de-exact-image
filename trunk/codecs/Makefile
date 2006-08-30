
ifeq "$(WITHLIBJPEG)" "1"
LDFLAGS += -ljpeg
else
NOT_SRCS += jpeg.cc
endif

ifeq "$(WITHLIBTIFF)" "1"
LDFLAGS += -ltiff
else
NOT_SRCS += tiff.cc
endif

ifeq "$(WITHLIBPNG)" "1"
LDFLAGS += -lpng
else
NOT_SRCS += png.cc
endif

ifeq "$(WITHLIBUNGIF)" "1"
LDFLAGS += -lungif
else
NOT_SRCS += gif.cc
endif

ifeq "$(WITHNETPBM)" "1"
LDFLAGS += -lnetpbm
else
NOT_SRCS += pnm.cc
endif

ifeq "$(WITHJASPER)" "1"
LDFLAGS += -ljasper
else
NOT_SRCS += jpeg2000.cc
endif

ifeq "$(WITHOPENEXR)" "1"
CPPFLAGS += $(OPENEXRINCS)
LDFLAGS += $(OPENEXRLIBS)
else
NOT_SRCS += openexr.cc
endif

ifeq "$(WITHLCMS)" "1"
LDFLAGS += -llcms
endif

include build/top.make

BINARY = lib
BINARY_EXT = $(X_LIBEXT)
DEPS =

CPPFLAGS += -I codecs/

DEPS = $(X_OUTARCH)/libbmp/bmp$(X_OBJEXT)

include build/bottom.make

