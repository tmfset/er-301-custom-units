%module strike_libstrike
%include <od/glue/mod.cpp.swig>

%{

#undef SWIGLUA

#include <Sieve.h>
#include <Bique.h>
#include <Arc.h>
#include <Fin.h>
#include <Strike.h>
#include <Tanh.h>
#include <Formant.h>
#include <Softy.h>
#include <MultiOptionView.h>
#include <CPR.h>
#include <CompressorScope.h>

#define SWIGLUA

%}

%include <Sieve.h>
%include <Bique.h>
%include <Arc.h>
%include <Fin.h>
%include <Strike.h>
%include <Tanh.h>
%include <Formant.h>
%include <Softy.h>
%include <MultiOptionView.h>
%include <CPR.h>
%include <CompressorScope.h>