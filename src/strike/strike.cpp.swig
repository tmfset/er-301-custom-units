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
#include <MultiOptionView.h>

#define SWIGLUA

%}

%include <Sieve.h>
%include <Bique.h>
%include <Arc.h>
%include <Fin.h>
%include <Strike.h>
%include <Tanh.h>
%include <MultiOptionView.h>