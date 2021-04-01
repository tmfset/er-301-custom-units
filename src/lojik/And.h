#pragma once

#include <od/objects/Object.h>
#include <sense.h>

namespace lojik {
  class And : public od::Object {
    public:
      And();
      virtual ~And();

#ifndef SWIGLUA
      virtual void process();
      od::Inlet  mIn   { "In" };
      od::Inlet  mGate { "Gate" };
      od::Outlet mOut  { "Out" };

      od::Option mSense { "Sense", INPUT_SENSE_LOW };
#endif
  };
}
