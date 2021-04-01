#pragma once

#include <od/objects/Object.h>
#include <sense.h>

namespace lojik {
  class Trig : public od::Object {
    public:
      Trig();
      virtual ~Trig();

#ifndef SWIGLUA
      virtual void process();
      od::Inlet  mIn  { "In" };
      od::Outlet mOut { "Out" };

      od::Option mSense { "Sense", INPUT_SENSE_LOW };
#endif

    private:
      float mCurrent = 0.0f;
      bool mAllow = true;
  };
}
