
include build/top.make

BINARY = ExactImage
BINARY_EXT = $(X_DYNEXT)
DEPS = $(lib_BINARY) $(codecs_BINARY) $($(X_MODULE)_OUTPUT)/api-perl-wrap.cc

CPPFLAGS += -I api $(PERLINCS)

$($(X_MODULE)_OUTPUT)/api-perl-wrap.cc: $(X_MODULE)/api.hh $(X_MODULE)/api-swig.hh
	swig -perl -c++ -outdir $($(X_MODULE)_OUTPUT) -o '$@' $(X_MODULE)/api-swig.hh

include build/bottom.make

