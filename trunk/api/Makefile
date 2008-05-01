include build/top.make

BINARY = ExactImage
BINARY_EXT = $(X_LIBEXT)
DEPS = 

# we have an own install
_X_BUILD_IMPLICIT := $(_X_BUILD_IMPLICIT)
X_BUILD_IMPLICIT := 0
include build/bottom.make
X_BUILD_IMPLICIT := $(_X_BUILD_IMPLICIT)
