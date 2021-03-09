#pragma once

#include <od/objects/Object.h>

namespace lojik {
  class DLatch : public od::Object {
    public:
      DLatch();
      virtual ~DLatch();

  #ifndef SWIGLUA
    virtual void process();
    od::Inlet mIn{"In"};
    od::Inlet mClock{"Clock"};
    od::Inlet mReset{"Reset"};
    od::Outlet mOut{"Out"};
  #endif

    private:
      float mCurrent = 0.0f;
      bool mCatch = true;
  };
}