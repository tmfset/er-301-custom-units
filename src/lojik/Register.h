
#pragma once

#include <RegisterState.h>
#include <OneTime.h>
#include <od/objects/Object.h>
#include <od/objects/StateMachine.h>
#include <od/constants.h>

namespace lojik {

  #define MODE_NORMAL 1
  #define MODE_SEQ    2

  class Register : public od::Object {
    public:
      Register(int max, float randomize);
      virtual ~Register();

#ifndef SWIGLUA
      virtual void process();
      void processNormal();
      void processSeq();

      od::Inlet  mIn      { "In" };
      od::Outlet mOut     { "Out" };

      od::Inlet  mLength  { "Length" };
      od::Inlet  mStride  { "Stride" };

      od::Inlet  mClock   { "Clock" };
      od::Inlet  mCapture { "Capture" };
      od::Inlet  mShift   { "Shift" };
      od::Inlet  mReset   { "Reset" };

      od::Parameter mScatter   { "Scatter",    0.0f };
      od::Parameter mDrift     { "Drift",      0.0f };
      od::Parameter mInputGain { "Input Gain", 1.0f };
      od::Parameter mInputBias { "Input Bias", 0.0f };

      od::Option mMode { "Mode", MODE_NORMAL };
      od::Option mSync { "Sync", 0b111 };

      const RegisterState& state() {
        return mState;
      }
#endif

      int   getMax()           { return mState.max(); }
      int   getLength()        { return mState.limit(); }
      int   getSeqLength()     { return mSequenceLength; }
      int   getStep()          { return mState.step(); }
      int   getShift()         { return mState.shift(); }
      float getData(int32_t i) { return mState.data(i); }

      void setMax(int v)           { mBuffer.setMax(v); }
      void setSeqLength(int v)     { mSequenceLength = v; }
      void setStep(int v)          { mBuffer.setStep(v); }
      void setShift(int v)         { mBuffer.setShift(v); }
      void setData(int i, float v) { mBuffer.setData(i, v); }

      void triggerDeserialize()     { mDeserialize = true; }

      void triggerZeroWindow()      { mState.markZeroWindow(); }
      void triggerScatterWindow()   { mState.markScatterWindow(); }
      void triggerRandomizeWindow() { mState.markRandomizeWindow(); }

      void triggerZeroAll()         { mState.markZeroAll(); }
      void triggerScatterAll()      { mState.markScatterAll(); }
      void triggerRandomizeAll()    { mState.markRandomizeAll(); }

    private:
      RegisterState mState;
      RegisterState mBuffer;

      OneTime mClockSwitch;
      OneTime mShiftSwitch;
      OneTime mCaptureSwitch;
      OneTime mResetSwitch;

      int mSequenceLength = 0;
      bool mRecordSequence = false;
      bool mDeserialize = false;

      void processTriggers() {
        mState.processTriggers();

        if (mDeserialize) {
          mState       = mBuffer;
          mBuffer      = {};
          mDeserialize = false;
        }
      }
  };
}