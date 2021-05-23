%module strike_libstrike
%include <od/glue/mod.cpp.swig>

%{

#undef SWIGLUA

#include <StateVariableFilter.h>
#include <ADEnvelope.h>
#include <FinOscillator.h>
#include <LowPassGate.h>

#define SWIGLUA

%}

%include <StateVariableFilter.h>
%include <ADEnvelope.h>
%include <FinOscillator.h>
%include <LowPassGate.h>
