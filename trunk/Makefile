
X_BUILD_IMPLICIT=0
include utility/Makefile
X_BUILD_IMPLICIT=1

CXXFLAGS += -Wall

MODULES = libbmp lib bin
include $(addsuffix /Makefile,$(MODULES))

