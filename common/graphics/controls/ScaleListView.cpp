#include <graphics/controls/ScaleListView.h>

namespace graphics {
  ScaleListView::ScaleListView(HasScaleBook &data) :
    od::Graphic(0, 0, 40, 40),
    mView(data) { }

  ScaleListView::~ScaleListView() { }
}