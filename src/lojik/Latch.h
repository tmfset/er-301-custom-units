#pragma once

#include <od/objects/Object.h>

namespace lojik {
  class Latch : public od::Object {
    public:
      Latch();
      virtual ~Latch();

  #ifndef SWIGLUA
    virtual void process();
    od::Inlet mSet{"Set"};
    od::Inlet mReset{"Reset"};
    od::Outlet mOut{"Out"};
  #endif

    private:
      float mCurrent = 0.0f;
  };
}