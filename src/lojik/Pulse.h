#pragma once

#include <od/objects/Object.h>

namespace lojik {
  class Pulse : public od::Object {
    public:
      Pulse();
      virtual ~Pulse();

  #ifndef SWIGLUA
    virtual void process();
    od::Inlet  mVPO   { "V/Oct" };
    od::Inlet  mFreq  { "Frequency" };
    od::Inlet  mSync  { "Sync" };
    od::Inlet  mWidth { "Width" };
    od::Outlet mOut   { "Out" };

    od::Parameter mPhase { "Phase", 0.0f };
  #endif
  };
}
