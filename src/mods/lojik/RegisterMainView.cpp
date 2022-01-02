#include "RegisterMainView.h"

namespace lojik {
  RegisterMainView::RegisterMainView(Register2 &data, int left, int bottom, int width, int height) :
      od::Graphic(left, bottom, width, height),
      mChart(data),
      mCircleChart(data, 0.5),
      mKeyboard(data, 1),
      mScaleList(data),
      mOffsetReadout(*data.getParameter("Offset")),
      mShiftReadout(*data.getParameter("Shift")),
      mLengthReadout(*data.getParameter("Length")),
      mStrideReadout(*data.getParameter("Stride")) { }
}