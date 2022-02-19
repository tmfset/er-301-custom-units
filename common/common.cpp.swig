
%{
#undef SWIGLUA

#include <graphics/interfaces/all.h>
#include <graphics/PagedGraphic.h>

#include <graphics/controls/MainControl.h>
#include <graphics/controls/ReadoutView.h>
#include <graphics/controls/ScaleListView.h>

#include <ui/DialMap.h>
#include <ui/LinearDialMap.h>

#include <util/Units.h>

#define SWIGLUA
%}

%include <graphics/interfaces/all.h>
%include <graphics/PagedGraphic.h>
%include <ui/DialMap.h>
%include <ui/LinearDialMap.h>
%include <graphics/controls/MainControl.h>
%include <graphics/controls/ReadoutView.h>
%include <graphics/controls/ScaleListView.h>
%include <util/Units.h>
