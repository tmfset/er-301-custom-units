%module polygon_libpolygon
%include <od/glue/mod.cpp.swig>

%{

#undef SWIGLUA

#include <Polygon.h>
#include <RoundRobinView.h>
#include <PagedSubView.h>

#define SWIGLUA

%}

%include <Polygon.h>
%include <RoundRobinView.h>
%include <PagedSubView.h>
%include <Observable.h>

%template(Quartet) polygon::Polygon<1>;
%template(Octet) polygon::Polygon<2>;
%template(Duodecet) polygon::Polygon<3>;