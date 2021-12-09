#pragma once

namespace common {
  class HasScaleData {
    public:
      HasScaleData();
      virtual ~HasScaleData();

      virtual int getScaleSize() = 0;
      virtual float getScaleCentValue(int i) = 0;

      virtual void attach() = 0;
      virtual void release() = 0;
  };
}