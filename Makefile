include config.make

X_OUTARCH := ./objdir

X_BUILD_IMPLICIT=0
include utility/Makefile
X_BUILD_IMPLICIT=1

# -s silcently corrupts binaries on OS X, sigh -ReneR
CFLAGS := -Wall -O2 # -O1 -ggdb # -fsanitize=address -fsanitize=undefined

# for config.h
CPPFLAGS += -I .

# -frename-registers and -funroll-loops brings a lot performance on
# my AMD Turion - about 20% time decrease (though it is included in -funroll-loops anyway) !!!

# from the linux-kernel build system:
cc-option = $(shell if $(CC) $(1) -S -o /dev/null -xc /dev/null \
            > /dev/null 2>&1; then echo "$(1)"; else echo "$(2)"; fi ;)

ifeq "$(X_ARCH)" "i686"
CFLAGS += -march=i686
CFLAGS += $(call cc-option,-mtune=pentium4,)
CFLAGS += $(call cc-option,-mfpmath=sse,)
endif

# TODO: improve to match i[3456]86
ifneq  "$(X_ARCH)" "i686"
CFLAGS += -fPIC
endif

ifeq "$(X_ARCH)" "sparc64"
CFLAGS += -mcpu=ultrasparc
CFLAGS += $(call cc-option,-mtune=niagara,)
endif

CFLAGS += $(call cc-option,-march=native)

CFLAGS += -funroll-loops -fomit-frame-pointer
CFLAGS += $(call cc-option,-funswitch-loops,)
CFLAGS += $(call cc-option,-fpeel-loops,)
CFLAGS += $(call cc-option,-ftracer,)
CFLAGS += $(call cc-option,-funit-at-a-time,)
CFLAGS += $(call cc-option,-frename-registers,)
CFLAGS += $(call cc-option,-ftree-vectorize,)

# we have some unimplemented colorspaces in the Image::iterator :-(
CFLAGS += $(call cc-option,-Wno-switch -Wno-switch-enum,)

CXXFLAGS := $(CFLAGS) -Wno-sign-compare

ifeq "$(STATIC)" "1"
X_EXEFLAGS += -static
endif

MODULES = lib codecs bardecode frontends ContourMatching
include $(addsuffix /Makefile,$(MODULES))

ifeq "$(WITHX11)" "1"
ifeq "$(WITHEVAS)" "1"
include gfx/Makefile
include edisplay/Makefile
endif
endif

ifeq "$(WITHSWIG)" "1"
include api/Makefile

ifeq "$(WITHLUA)" "1"
include api/lua/Makefile
endif
ifeq "$(WITHPERL)" "1"
include api/perl/Makefile
endif
ifeq "$(WITHPHP)" "1"
include api/php/Makefile
endif
ifeq "$(WITHPYTHON)" "1"
include api/python/Makefile
endif
ifeq "$(WITHRUBY)" "1"
#include api/ruby/Makefile
endif

endif

check: $(X_OUTARCH)/econvert/econvert$(X_EXEEXT) $(X_OUTARCH)/edentify/edentify$(X_EXEEXT)
	$(Q)cd testsuite; ./run ../$(X_OUTARCH)/econvert/econvert$(X_EXEEXT)
