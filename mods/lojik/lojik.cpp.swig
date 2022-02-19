%module lojik_liblojik
%include <od/glue/mod.cpp.swig>
%include <common.cpp.swig>

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
#include <TLatch.h>
#include <Register.h>
#include <Register2.h>
#include <RegisterMainView.h>
#include <controls/main/QuantizerView.h>
#include <Euclid.h>
#include <EuclidCircle.h>
#include <Chance.h>

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
%include <TLatch.h>
%include <Register.h>
%include <Register2.h>
%include <RegisterMainView.h>
%include <controls/main/QuantizerView.h>
%include <Euclid.h>
%include <EuclidCircle.h>
%include <Chance.h>
