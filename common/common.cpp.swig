
%{
#undef SWIGLUA

#include <graphics/interfaces/all.h>
#include <graphics/PagedGraphic.h>

#include <graphics/composites/ParameterReadout.h>

#include <dsp/objects/CircularParameter.h>

#include <ui/DialMap.h>
#include <ui/LinearDialMap.h>

#include <util/Units.h>

#define SWIGLUA
%}

%include <graphics/interfaces/all.h>
%include <graphics/PagedGraphic.h>
%include <dsp/objects/CircularParameter.h>
%include <ui/DialMap.h>
%include <ui/LinearDialMap.h>
%include <graphics/composites/ParameterReadout.h>
%include <util/Units.h>
