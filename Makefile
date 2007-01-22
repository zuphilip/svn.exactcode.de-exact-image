include config.make

X_OUTARCH := ./objdir

X_BUILD_IMPLICIT=0
include utility/Makefile
X_BUILD_IMPLICIT=1

CFLAGS = -Wall -O2 -s
CXXFLAGS = -Wall -O2 -s -Wno-sign-compare
#CFLAGS = -Wall -O0 -ggdb
#CXXFLAGS = -Wall -O0 -ggdb

# for config.h
CPPFLAGS += -I .

# -frename-registers and -funroll-loops brings a lot performance on
# my AMD Turion - about 20% time decrease (though it is included in -funroll-loops anyway) !!!

# from the linux-kernel build system:
cc-option = $(shell if $(CC) $(1) -S -o /dev/null -xc /dev/null \
            > /dev/null 2>&1; then echo "$(1)"; else echo "$(2)"; fi ;)

ifeq "$(X_ARCH)" "i686"
CXXFLAGS += -march=i686
endif

ifeq "$(X_ARCH)" "sparc64"
CXXFLAGS += -mcpu=ultrasparc
CXXFLAGS += $(call cc-option,-mtune=niagara,)
endif

CXXFLAGS += -funroll-loops -fomit-frame-pointer
CXXFLAGS += $(call cc-option,-funswitch-loops,)
CXXFLAGS += $(call cc-option,-fpeel-loops,)
CXXFLAGS += $(call cc-option,-ftracer,)
CXXFLAGS += $(call cc-option,-funit-at-a-time,)
CXXFLAGS += $(call cc-option,-frename-registers,)
CXXFLAGS += $(call cc-option,-ftree-vectorize,)

#CXXFLAGS += $(call cc-option,-mfpmath=sse,)


MODULES = lib codecs econvert edentify
include $(addsuffix /Makefile,$(MODULES))

ifeq "$(WITHX11)" "1"
ifeq "$(WITHEVAS)" "1"
include gfx/Makefile
include edisplay/Makefile
endif
endif

ifeq "$(WITHSWIG)" "1"
include api/Makefile
endif

check: $(X_OUTARCH)/econvert/econvert$(X_EXEEXT) $(X_OUTARCH)/edentify/edentify$(X_EXEEXT)
	$(Q)cd testsuite; ./run ../$(X_OUTARCH)/econvert/econvert$(X_EXEEXT)
