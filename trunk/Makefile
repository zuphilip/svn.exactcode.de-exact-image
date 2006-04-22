
include config.make

X_BUILD_IMPLICIT=0
include utility/Makefile
X_BUILD_IMPLICIT=1

CFLAGS = -Wall -O0 -ggdb
CXXFLAGS = -Wall -O0 -ggdb

# -frename-registers and -funroll-loops brings a lot performance on
# my AMD Turion - about 20% time decrease (though it is included in -funroll-loops anyway) !!!
CXXFLAGS += -O2 -ftree-vectorize -ftracer -frename-registers -funit-at-a-time -funroll-loops -fpeel-loops # -mfpmath=sse

MODULES = libbmp lib econvert
include $(addsuffix /Makefile,$(MODULES))

ifeq "$(WITHX11)" "1"
ifeq "$(WITHEVAS)" "1"
include gfx/Makefile
include edisplay/Makefile
endif
endif
