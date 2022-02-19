#include "RegisterMainView.h"

namespace lojik {
  RegisterMainView::RegisterMainView(Register2 &data) :
      graphics::MainControl(1),
      mChart(data),
      mCircleChart(data, 0.5),
      mKeyboard(data),
      mScaleList(data),
      mOffsetReadout(data.getParameter("Offset")),
      mShiftReadout(data.getParameter("Shift")),
      mLengthReadout(data.getParameter("Length")),
      mStrideReadout(data.getParameter("Stride")) {
    //configureReadout(mOffsetReadout);
    //configureReadout(mShiftReadout);
    configureReadout(mLengthReadout);
    configureReadout(mStrideReadout);
    configureReadouts();
  }
}