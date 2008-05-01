include build/top.make

BINARY = ExactImage
BINARY_EXT = $(X_DYNEXT)
DEPS = $(lib_BINARY) $(bardecode_BINARY) $(codecs_BINARY) $(api_BINARY) $($(X_MODULE)_OUTPUT)/api-php-wrap.cc

objdir/api/php/CXXFLAGS := -I api $(PHPINCS) -fno-strict-aliasing

$($(X_MODULE)_OUTPUT)/api-php-wrap.cc: $(X_MODULE)/../api.hh $(X_MODULE)/../api-swig.hh
	$(Q)echo "  SWIG      $($(X_MODULE)_OUTPUT)"
	$(Q)swig -php5 -c++ -outdir $($(X_MODULE)_OUTPUT) -o '$@' $(X_MODULE)/../api-swig.hh

# we have an own install
_X_BUILD_IMPLICIT := $(_X_BUILD_IMPLICIT)
X_BUILD_IMPLICIT := 0
include build/bottom.make
X_BUILD_IMPLICIT := $(_X_BUILD_IMPLICIT)

# install
all:: $($(X_MODULE)_BINARY)
install:: $($(X_MODULE)_BINARY)
	$(Q)echo "INSTALL PHP module $($(X_MODULE)_OUTPUT)/..."
	$(Q)mkdir -p $(libdir)/php/...
	$(Q)install $($(X_MODULE)_OUTPUT)/ExactImage.pm $($(X_MODULE)_BINARY) $(libdir)/php/...
