#include "RegisterMainView.h"

namespace lojik {
  RegisterMainView::RegisterMainView(Register2 &data, int left, int bottom, int width, int height) :
      od::Graphic(left, bottom, width, height),
      mChart(data),
      mCircleChart(data, 0.5),
      mKeyboard(data),
      mScaleList(data),
      mOffsetReadout(0, 0, 20, 8),
      mShiftReadout(0, 0, 20, 8),
      mLengthReadout(0, 0, 20, 8),
      mStrideReadout(0, 0, 20, 8) {
    configureReadout(mOffsetReadout, data.getParameter("Offset"));
    configureReadout(mShiftReadout, data.getParameter("Shift"));
    configureReadout(mLengthReadout, data.getParameter("Length"));
    configureReadout(mStrideReadout, data.getParameter("Stride"));
  }
}