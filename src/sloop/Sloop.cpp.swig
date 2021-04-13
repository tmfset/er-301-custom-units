%module sloop_libsloop
%include <od/glue/mod.cpp.swig>

%{

#undef SWIGLUA

#include <Sloop.h>
#include <SloopHeadDisplay.h>

#define SWIGLUA

%}

%include <Sloop.h>
%include <SloopHeadDisplay.h>
