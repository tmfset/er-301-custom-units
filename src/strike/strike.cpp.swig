%module strike_libstrike
%include <od/glue/mod.cpp.swig>

%{

#undef SWIGLUA

#include <Filter.h>
#include <StateVariableFilter.h>

#define SWIGLUA

%}

%include <Filter.h>
%include <StateVariableFilter.h>
