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
      float* mData;

      bool mWait = false;

      uint32_t index(uint32_t limit);
      void step(uint32_t limit);
      void shift(uint32_t limit);
      void reset();

      uint32_t mStepCount  = 0;
      uint32_t mShiftCount = 0;
  };
}