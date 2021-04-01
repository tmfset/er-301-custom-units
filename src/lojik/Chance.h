#pragma once

#include <od/objects/Object.h>
#include <OneTime.h>
#include <sense.h>

#define CHANCE_MODE_TRIGGER 1
#define CHANCE_MODE_GATE 2
#define CHANCE_MODE_PASSTHROUGH 3

namespace lojik {
  class Chance : public od::Object {
    public:
      Chance();
      virtual ~Chance();

#ifndef SWIGLUA
      virtual void process();
      od::Inlet  mIn     { "In" };
      od::Inlet  mChance { "Chance" };
      od::Outlet mOut    { "Out" };

      od::Option mMode { "Mode", CHANCE_MODE_GATE };
      od::Option mSense { "Sense", INPUT_SENSE_LOW };
#endif

    private:
      OneTime mTrigSwitch;
      bool mAllow = false;
  };
}
