%module polygon_libpolygon
%include <od/glue/mod.cpp.swig>

%{

#undef SWIGLUA

#include <Polygon.h>

#define SWIGLUA

%}

%include <Polygon.h>
