
include build/top.make

BINARY = optimize2bw empty-page edge

BINARY_EXT = $(X_EXEEXT)
DEPS = $(lib_BINARY)

CPPFLAGS += -I utility

include build/bottom.make

all:: $($(X_MODULE)_BINARY)

# build rules

$(X_MODULE): $($(X_MODULE)_BINARY)

$($(X_MODULE)_OUTPUT)/empty-page$(X_EXEEXT): $($(X_MODULE)_OUTPUT)/empty-page.o $(X_OUTARCH)/utility/ArgumentList$(X_OBJEXT) $(lib_BINARY)
	@echo '  LINK EXEC $@'
	$(Q)$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -o '$@' $^ $(LDFLAGS)

$($(X_MODULE)_OUTPUT)/optimize2bw$(X_EXEEXT): $($(X_MODULE)_OUTPUT)/optimize2bw.o $(X_OUTARCH)/utility/ArgumentList$(X_OBJEXT) $(lib_BINARY)
	@echo '  LINK EXEC $@'
	$(Q)$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -o '$@' $^ $(LDFLAGS)

$($(X_MODULE)_OUTPUT)/edge$(X_EXEEXT): $($(X_MODULE)_OUTPUT)/edge.o $(X_OUTARCH)/utility/ArgumentList$(X_OBJEXT) $(lib_BINARY)
	@echo '  LINK EXEC $@'
	$(Q)$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -o '$@' $^ $(LDFLAGS)


