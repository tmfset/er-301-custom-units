#pragma once

#include <od/objects/Object.h>

//#define REGISTER_MAX_SIZE 128

namespace lojik {
  class Register : public od::Object {
    public:
      Register();
      virtual ~Register();

  #ifndef SWIGLUA
    virtual void process();
    od::Inlet mIn{"In"};
    od::Outlet mOut{"Out"};

    od::Inlet mLength{"Length"};
    od::Inlet mOrigin {"Origin"};

    od::Inlet mCapture{"Capture"};
    od::Inlet mShift{"Shift"};
    od::Inlet mStep{"Step"};

    od::Inlet mZero{"Zero"};
    od::Inlet mZeroAll{"ZeroAll"};
    od::Inlet mRandomize{"Randomize"};
    od::Inlet mRandomizeAll{"RandomizeAll"};
    od::Inlet mRandomizeGain{"RandomizeGain"};
  #endif

    private:
      float* mData;
      // float mData[REGISTER_MAX_SIZE];

      bool canStep  = true;
      bool canShift = true;
      bool canOrigin = true;
      bool canCapture = true;

      // Keep track of where we are in mData.
      uint32_t mIndex = 0;

      // Keep tack of where we are after any shifts.
      uint32_t mOffset = 0;
  };
}