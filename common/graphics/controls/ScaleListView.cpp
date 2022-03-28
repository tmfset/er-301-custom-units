#include <graphics/controls/ScaleListView.h>

namespace graphics {
  ScaleListView::ScaleListView(HasScaleBook &data, od::Followable &followable) :
    od::Graphic(0, 0, 40, 40),
    mValue(followable),
    mView(data) {
      mCursorState.orientation = od::cursorRight;
    }

  ScaleListView::~ScaleListView() { }
}