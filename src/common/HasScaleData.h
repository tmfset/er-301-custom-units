#pragma once

namespace common {
  class HasScaleData {
    public:
      HasScaleData();
      virtual ~HasScaleData();

      virtual int getScaleSize() = 0;
      virtual float getScaleCentValue(int i) = 0;

      virtual float getDetectedCentValue() = 0;
      virtual int   getDetectedOctaveValue() = 0;
      virtual float getQuantizedCentValue() = 0;

      virtual void attach() = 0;
      virtual void release() = 0;
  };
}