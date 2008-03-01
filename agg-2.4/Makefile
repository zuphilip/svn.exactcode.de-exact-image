include build/top.make

SRCS = $(filter-out $(NOT_SRCS), $(notdir $(wildcard $(X_MODULE)/src/*.cpp )))

CPPFLAGS += -I agg-2.4/include

ifeq "$(WITHFREETYPE)" "1"
SRCS += agg_font_freetype.cpp
CPPFLAGS += $(FREETYPEINCS)
CPPFLAGS += -I agg-2.4/font_freetype
endif

BINARY = libagg
BINARY_EXT = $(X_LIBEXT)

$($(X_MODULE)_OUTPUT)/%.o: $(X_MODULE)/src/%.cpp
	@echo '  C++       $@'
	$(Q)$(COMPILE.cc) $($(X_MODULE)_CFLAGS) -MMD -MP -o '$@' '$<'

$($(X_MODULE)_OUTPUT)/%.o: $(X_MODULE)/font_freetype/%.cpp
	@echo '  C++       $@'
	$(Q)$(COMPILE.cc) $($(X_MODULE)_CFLAGS) -MMD -MP -o '$@' '$<'

include build/bottom.make
