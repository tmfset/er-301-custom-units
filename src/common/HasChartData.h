#pragma once

namespace common {
  class HasChartData {
    public:
      HasChartData();
      virtual ~HasChartData();

      virtual int length() = 0;
      virtual int current() = 0;
      virtual float value(int i) = 0;

      virtual void attach() = 0;
      virtual void release() = 0;
  };
}