
include config.make

X_BUILD_IMPLICIT=0
include utility/Makefile
X_BUILD_IMPLICIT=1

# -frename-registers and -funroll-loops brings a lot performance on
# my AMD Turion - about 20% time decrease !!! 
CXXFLAGS += -Wall -O2 -march=athlon64 -ggdb -ftree-vectorize -ftracer -frename-registers -funit-at-a-time -funroll-loops -fpeel-loops -mfpmath=sse

CFLAGS = -Wall -O0 -ggdb
CXXFLAGS = -Wall -O0 -ggdb

MODULES = libbmp lib bin
include $(addsuffix /Makefile,$(MODULES))

ifeq "$(WITHX11)" "1"
ifeq "$(WITHEVAS)" "1"
include gfx/Makefile
include viewer/Makefile
endif
endif
