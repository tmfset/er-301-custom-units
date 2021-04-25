%module strike_libstrike
%include <od/glue/mod.cpp.swig>

%{

#undef SWIGLUA

#include <Biquad.h>

#define SWIGLUA

%}

%include <Biquad.h>
