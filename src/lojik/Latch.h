#pragma once

#include <od/objects/Object.h>
#include <sense.h>

namespace lojik {
  class Latch : public od::Object {
    public:
      Latch();
      virtual ~Latch();

#ifndef SWIGLUA
      virtual void process();
      od::Inlet  mIn    { "In" };
      od::Inlet  mReset { "Reset" };
      od::Outlet mOut   { "Out" };

      od::Option mSense { "Sense", INPUT_SENSE_LOW };
#endif

    private:
      float mCurrent = 0.0f;
  };
}