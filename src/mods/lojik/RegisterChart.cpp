#include "RegisterChart.h"

namespace lojik {
  RegisterChart::RegisterChart(common::HasChartData &data, int left, int bottom, int width, int height) :
      od::Graphic(left, bottom, width, height),
      mChart(data, 3, 2) { }
}