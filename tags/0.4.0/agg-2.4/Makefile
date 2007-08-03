include build/top.make

SRCS = $(filter-out $(NOT_SRCS), $(notdir $(wildcard $(X_MODULE)/src/*.cpp )))

BINARY = libagg
BINARY_EXT = $(X_LIBEXT)

CPPFLAGS += -I agg-2.4/include #$(shell pkg-config evas --atleast-version 0.9.9.027 && echo -D WITHEVAS_COLOR_PREMUL)
LDFLAGS += #$(X11LIBS) -lXrender $(EVASLIBS)

include build/bottom.make

$($(X_MODULE)_OUTPUT)/%.o: $(X_MODULE)/src/%.cpp
	@echo '  C++       $@'
	$(Q)$(COMPILE.cc) $($(X_MODULE)_CFLAGS) -MMD -MP -o '$@' '$<'
