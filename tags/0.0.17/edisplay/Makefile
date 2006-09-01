
include build/top.make

BINARY = edisplay
BINARY_EXT = $(X_EXEEXT)

CPPFLAGS += -I . -I lib -I gfx

DEPS += $(X_OUTARCH)/gfx/X11Helper$(X_OBJEXT) $(X_OUTARCH)/gfx/EvasHelper$(X_OBJEXT) $(X_OUTARCH)/utility/Timer$(X_OBJEXT)

include build/bottom.make

