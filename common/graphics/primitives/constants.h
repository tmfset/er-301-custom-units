#pragma once

namespace graphics {
  struct JustifyAlign {
    inline JustifyAlign(od::Justification j, od::Alignment a) :
      justify(j), align(a) { }

    od::Justification justify;
    od::Alignment align;
  };

  #define LEFT_BOTTOM   graphics::JustifyAlign(od::justifyLeft,   od::alignBottom)
  #define LEFT_MIDDLE   graphics::JustifyAlign(od::justifyLeft,   od::alignMiddle)
  #define LEFT_TOP      graphics::JustifyAlign(od::justifyLeft,   od::alignTop)
  #define CENTER_BOTTOM graphics::JustifyAlign(od::justifyCenter, od::alignBottom)
  #define CENTER_MIDDLE graphics::JustifyAlign(od::justifyCenter, od::alignMiddle)
  #define CENTER_TOP    graphics::JustifyAlign(od::justifyCenter, od::alignTop)
  #define RIGHT_BOTTOM  graphics::JustifyAlign(od::justifyRight,  od::alignBottom)
  #define RIGHT_MIDDLE  graphics::JustifyAlign(od::justifyRight,  od::alignMiddle)
  #define RIGHT_TOP     graphics::JustifyAlign(od::justifyRight,  od::alignTop)
}