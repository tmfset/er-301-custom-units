#pragma once

#include <od/objects/Object.h>

namespace lojik {
  class Curl : public od::Object {
    public:
      Curl();
      virtual ~Curl();

  #ifndef SWIGLUA
    virtual void process();
    od::Inlet  mIn   { "In" };
    od::Inlet  mGain { "Gain" };
    od::Inlet  mBias { "Bias" };
    od::Inlet  mFold { "Fold" };
    od::Outlet mOut  { "Out" };
  #endif
  };
}
