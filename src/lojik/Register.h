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
    od::Inlet  mIn      { "In" };
    od::Outlet mOut     { "Out" };
    od::Inlet  mLength  { "Length" };
    od::Inlet  mClock   { "Clock" };
    od::Inlet  mCapture { "Capture" };
    od::Inlet  mShift   { "Shift" };
    od::Inlet  mReset   { "Reset" };
    od::Inlet  mScatter { "Scatter" };
    od::Inlet  mGain    { "Gain" };
  #endif

    private:
      bool mWait = false;

      int32_t index(int32_t limit);
      int32_t clamp(int32_t value);

      void step(int32_t limit);
      void shift(int32_t limit);
      void reset();

      int32_t mStepCount  = 0;
      int32_t mShiftCount = 0;
  };
}