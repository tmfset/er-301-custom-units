#include "QuantizerView.h"

namespace lojik {
  QuantizerView::QuantizerView(
    graphics::Quantizer &data
  ) :
    graphics::MainControl(1),
    mKeyboard(data),
    mScaleList(data) { }
}