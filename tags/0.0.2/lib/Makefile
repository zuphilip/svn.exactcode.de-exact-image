
include build/top.make

BINARY = lib
BINARY_EXT = $(X_LIBEXT)
DEPS =

CPPFLAGS += -I lib/
LDFLAGS += -ljpeg -ltiff -lpng -ljasper

DEPS = $(X_OUTARCH)/libbmp/bmp$(X_OBJEXT)

include build/bottom.make

