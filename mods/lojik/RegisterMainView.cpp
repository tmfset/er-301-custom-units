#include "RegisterMainView.h"

namespace lojik {
  RegisterMainView::RegisterMainView(Register2 &data) :
      graphics::MainControl(2),
      mChart(data),
      mCircleChart(data, 0.5),
      mKeyboard(data),
      mScaleList(data) {
    mReadoutList.addItem("Length", data.getParameter("Length"));
    mReadoutList.addItem("Stride", data.getParameter("Stride"));
    mReadoutList.addItem("Shift", data.getParameter("Shift"));
    mReadoutList.addItem("Offset", data.getParameter("Offset"));
    mGateList.addItem("reset");
    mGateList.addItem("write");
    mGateList.addItem("shift");
  }
}