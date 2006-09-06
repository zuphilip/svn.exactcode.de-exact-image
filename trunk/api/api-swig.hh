
%module ExactImage
%include "cstring.i"

# manually include it, otherwise SWIG will not source it
%include "config.h"

%{
#include "api.hh"
%}

/* Parse the header file to generate wrappers */
%include "api.hh"
