#include "RegisterCircle.h"

namespace lojik {
  RegisterCircle::RegisterCircle(common::HasChartData &data, int left, int bottom, int width, int height) :
      od::Graphic(left, bottom, width, height),
      mChart(data, 0.5) { }
}