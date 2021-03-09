%module lojik_liblojik
%include <od/glue/mod.cpp.swig>

%{

#undef SWIGLUA

#include <And.h>
#include <Or.h>
#include <Not.h>
#include <Latch.h>
#include <DLatch.h>
#include <Trig.h>

#define SWIGLUA

%}

%include <And.h>
%include <Or.h>
%include <Not.h>
%include <Latch.h>
%include <DLatch.h>
%include <Trig.h>
