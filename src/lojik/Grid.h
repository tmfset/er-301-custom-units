#pragma once

#include <od/objects/Object.h>

#define MAX_WIDTH 128
#define MAX_HEIGHT 128

namespace lojik {
  class Grid : public od::Object {
    public:
      Grid();
      virtual ~Grid();

  #ifndef SWIGLUA
    virtual void process();
    od::Inlet mIn{"In"};

    od::Inlet mShift{"Shift"};
    od::Inlet mShiftX{"ShiftX"};
    od::Inlet mShiftY{"ShiftY"};

    od::Inlet mNext{"Next"};
    od::Inlet mNextX{"NextX"};
    od::Inlet mNextY{"NextY"};

    od::Inlet mClock{"Clock"};
    od::Inlet mReset{"Origin"};
    od::Outlet mOut{"Out"};
  #endif

    void randomize();

    private:
      uint32_t xOrigin = 0, yOrigin = 0;
      uint32_t xWrap = 4, yWrap = 4;

      float mData[MAX_WIDTH][MAX_HEIGHT];
  };
}