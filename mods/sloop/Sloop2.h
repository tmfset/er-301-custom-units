#pragma once

#include <od/objects/heads/TapeHead.h>

namespace sloop {
  class Sloop2 : public od::TapeHead {
    public:
      Sloop2(bool stereo) {
        
      }

#ifndef SWIGLUA
      virtual void process();

      od::Inlet mTap       { "Tap" };
      od::Inlet mEngage    { "Engage" };
      od::Inlet mClear     { "Clear" };
      od::Inlet mWrite     { "Write" };
      od::Inlet mReset     { "Reset" };

      od::Outlet mResetOut { "Reset Out" };
      od::Outlet mClockOut { "Clock Out" };

      od::Parameter mLength   { "Length", 4 };
      od::Parameter mThrough  { "Through", 1.0 };
      od::Parameter mFeedback { "Feedback", 1.0 };
      od::Parameter mFade     { "Fade", 0.005 };
      od::Parameter mResetTo  { "Reset To", 0 };
#endif
    private:
      inline void processInternal() {
        const float *tap    = mTap.buffer();
        const float *engage = mEngage.buffer();
        const float *clear  = mClear.buffer();
        const float *write  = mWrite.buffer();
        const float *reset  = mReset.buffer();
      }
  };
}