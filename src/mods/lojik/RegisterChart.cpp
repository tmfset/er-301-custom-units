#include "RegisterChart.h"

namespace lojik {
  RegisterChart::RegisterChart(common::HasChartData &chartData, int left, int bottom, int width, int height) :
      od::Graphic(left, bottom, width, height),
      mChart(chartData, 3, 2) { }
}