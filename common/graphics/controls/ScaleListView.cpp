#include <graphics/controls/ScaleListView.h>

namespace graphics {
  ScaleListView::ScaleListView(HasScaleBook &data, od::Parameter *param) :
    od::Graphic(0, 0, 40, 40),
    mDisplay(param),
    mView(data) {
      mCursorState.orientation = od::cursorRight;
    }

  ScaleListView::~ScaleListView() { }
}