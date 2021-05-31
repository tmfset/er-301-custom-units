%module strike_libstrike
%include <od/glue/mod.cpp.swig>

%{

#undef SWIGLUA

#include <Sieve.h>
#include <Bique.h>
#include <Arc.h>
#include <FinOscillator.h>
#include <LowPassGate.h>
#include <MultiOptionView.h>

#define SWIGLUA

%}

%include <Sieve.h>
%include <Bique.h>
%include <Arc.h>
%include <FinOscillator.h>
%include <LowPassGate.h>
%include <MultiOptionView.h>