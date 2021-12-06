#include "RegisterChart.h"

namespace lojik {
  RegisterChart::RegisterChart(common::HasChartData &regLike, int left, int bottom, int width, int height) :
      od::Graphic(left, bottom, width, height),
      mRegister(regLike) {
    mRegister.attach();
  }
}