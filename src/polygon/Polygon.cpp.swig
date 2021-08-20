%module polygon_libpolygon
%include <od/glue/mod.cpp.swig>

%{

#undef SWIGLUA

#include <Polygon.h>
#include <RoundRobinView.h>

#define SWIGLUA

%}

%include <Polygon.h>
%include <RoundRobinView.h>