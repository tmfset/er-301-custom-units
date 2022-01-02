#pragma once

#include <string>

namespace common {
  class HasChart {
    public:
      HasChart();
      virtual ~HasChart();

      virtual int getChartSize() = 0;
      virtual int getChartCurrentIndex() = 0;
      virtual int getChartBaseIndex() = 0;
      virtual float getChartValue(int i) = 0;

      virtual void attach() = 0;
      virtual void release() = 0;
  };

  class HasScale {
    public:
      HasScale();
      virtual ~HasScale();

      virtual std::string getScaleName() = 0;
      virtual int getScaleSize() = 0;
      virtual float getScaleCentValue(int i) = 0;

      virtual float getDetectedCentValue() = 0;
      virtual int getDetectedOctaveValue() = 0;
      virtual float getQuantizedCentValue() = 0;

      virtual void attach() = 0;
      virtual void release() = 0;
  };

  class HasScaleBook {
    public:
      HasScaleBook();
      virtual ~HasScaleBook();

      virtual int getScaleBookIndex() = 0;
      virtual int getScaleBookSize() = 0;
      virtual std::string getScaleName(int i) = 0;
      virtual int getScaleSize(int i) = 0;

      virtual void attach() = 0;
      virtual void release() = 0;
  };
}