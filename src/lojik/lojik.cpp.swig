%module lojik_liblojik
%include <od/glue/mod.cpp.swig>

%{

#undef SWIGLUA

#include <Pulse.h>
#include <And.h>
#include <Or.h>
#include <Not.h>
#include <Trig.h>
#include <Pick.h>
#include <Wait.h>
#include <Latch.h>
#include <DLatch.h>
#include <Register.h>
#include <Euclid.h>
#include <EuclidCircle.h>
#include <Chance.h>
#include <RegisterMainView.h>

#define SWIGLUA

%}

%include <Pulse.h>
%include <And.h>
%include <Or.h>
%include <Not.h>
%include <Trig.h>
%include <Pick.h>
%include <Wait.h>
%include <Latch.h>
%include <DLatch.h>
%include <Register.h>
%include <Euclid.h>
%include <EuclidCircle.h>
%include <Chance.h>
%include <RegisterMainView.h>
