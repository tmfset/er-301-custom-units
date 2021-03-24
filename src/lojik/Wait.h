#pragma once

#include <od/objects/Object.h>

namespace lojik {
  class Wait : public od::Object {
    public:
      Wait();
      virtual ~Wait();

  #ifndef SWIGLUA
    virtual void process();
    od::Inlet  mIn    { "In" };
    od::Inlet  mWait  { "Wait" };
    od::Inlet  mClock { "Clock" };
    od::Inlet  mReset { "Reset" };
    od::Outlet mOut   { "Out" };
  #endif

    private:
      bool mAllowStep = false;
      int mCount = 0;
  };
}
