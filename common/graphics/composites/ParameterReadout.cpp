#include <graphics/composites/ParameterReadout.h>

namespace graphics {
  ParameterReadout::ParameterReadout(od::Parameter *param) :
      od::Graphic(0, 0, 20, 20),
      mDisplay(param) {
    setCursorOrientation(od::cursorRight);
  }

  ParameterReadout::~ParameterReadout() { }
}