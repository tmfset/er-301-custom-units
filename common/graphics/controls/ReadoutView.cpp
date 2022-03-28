#include <graphics/controls/ReadoutView.h>

namespace graphics {
  ReadoutView::ReadoutView(od::Parameter *param) :
      od::Graphic(0, 0, 20, 20),
      mDisplay(*param) {
    setCursorOrientation(od::cursorRight);
  }

  ReadoutView::~ReadoutView() { }
}