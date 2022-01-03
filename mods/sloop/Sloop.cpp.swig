%module sloop_libsloop
%include <od/glue/mod.cpp.swig>

%{

#undef SWIGLUA

#include <ClockMarks.h>
#include <Sloop.h>
#include <SloopHeadMainDisplay.h>
#include <SloopHeadSubDisplay.h>

#define SWIGLUA

%}

%include <ClockMarks.h>
%include <Sloop.h>
%include <SloopHeadMainDisplay.h>
%include <SloopHeadSubDisplay.h>
