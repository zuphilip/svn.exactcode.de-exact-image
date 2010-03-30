
include build/top.make

BINARY = edisplay
BINARY_EXT = $(X_EXEEXT)

CPPFLAGS += -I . -I lib -I gfx $(shell pkg-config evas --atleast-version 0.9.9.027 && echo -D WITHEVAS_X11_CONNECTION)

DEPS += $(X_OUTARCH)/gfx/X11Helper$(X_OBJEXT) $(X_OUTARCH)/gfx/EvasHelper$(X_OBJEXT) $(X_OUTARCH)/utility/Timer$(X_OBJEXT) $(X_OUTARCH)/utility/ArgumentList$(X_OBJEXT) $(lib_BINARY) $(codecs_BINARY)

include build/bottom.make

