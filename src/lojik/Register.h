#pragma once

#include <util.h>

#include <od/extras/Random.h>
#include <od/objects/Object.h>
#include <hal/simd.h>
#include <vector>

#define REGISTER_MAX 1024

namespace lojik {

#ifndef SWIGLUA
  struct RegisterState {
    std::vector<float32_t> _data;
    int32_t   _step   = 0;
    int32_t   _shift  = 0;
    int32_t   _stride = 1;
    int32_t   _limit  = 0;
    float32_t _gain   = 1.0f;

    void setMax(uint32_t v) {
      _data.resize(clamp(v, 1, REGISTER_MAX), 0.0f);
    }

    inline void setLSG(int32_t l, int32_t s, float32_t g) {
      _limit  = clamp(l, 1, _data.size());
      _stride = s;
      _gain   = g;
    }

    inline int32_t index(int32_t base, int32_t offset) {
      return mod(base + offset, _limit);
    }

    inline int32_t current() {
      return this->index(_step, _shift);
    }

    inline void step()  { _step  = this->index(_step, _stride); }
    inline void shift() { _shift = this->index(_shift, _stride); }
    inline void reset() { _step  = 0; }

    inline void set(int32_t i, float32_t v) { _data[i] = v * _gain; }
    inline float32_t get(int32_t i) { return _data[i]; }

    inline void randomize(int32_t i) { set(i, od::Random::generateFloat(-1.0f, 1.0f)); }
    inline void zero(int32_t i)      { set(i, 0); }

    inline void randomizeWindow() { setWindow(&RegisterState::randomize); }
    inline void zeroWindow()      { setWindow(&RegisterState::zero); }
    inline void randomizeAll()    { setAll(&RegisterState::randomize); }
    inline void zeroAll()         { setAll(&RegisterState::zero); }

    inline void setWindow(void (RegisterState::*f)(int32_t)) {
      uint32_t start   = current();
      uint32_t current = start;
      do {
        (this->*f)(current);
        current = this->index(current, _stride);
      } while (start != current);
    }

    inline void setAll(void (RegisterState::*f)(int32_t)) {
      for (uint32_t i = 0; i < _data.size(); i++) (this->*f)(i);
    }
  };
#endif

  class Register : public od::Object {
    public:
      Register(int max, bool randomize);
      virtual ~Register();

#ifndef SWIGLUA
      virtual void process();
      void processTriggers();

      od::Inlet  mIn      { "In" };
      od::Outlet mOut     { "Out" };
      od::Inlet  mLength  { "Length" };
      od::Inlet  mStride  { "Stride" };
      od::Inlet  mClock   { "Clock" };
      od::Inlet  mCapture { "Capture" };
      od::Inlet  mShift   { "Shift" };
      od::Inlet  mReset   { "Reset" };
      od::Inlet  mScatter { "Scatter" };
      od::Inlet  mGain    { "Gain" };
#endif

      int getMax()             { return mState._data.size(); }
      int getStep()            { return mState._step; }
      int getShift()           { return mState._shift; }
      float getData(int32_t i) { return mState.get(i); }

      void setMax(int v)           { mBuffer.setMax(v); }
      void setStep(int v)          { mBuffer._step    = v; }
      void setShift(int v)         { mBuffer._shift   = v; }
      void setData(int i, float v) { mBuffer._data[i] = v; }

      void triggerDeserialize()     { mTriggerDeserialize     = true; }
      void triggerZeroWindow()      { mTriggerZeroWindow      = true; }
      void triggerZeroAll()         { mTriggerZeroAll         = true; }
      void triggerRandomizeWindow() { mTriggerRandomizeWindow = true; }
      void triggerRandomizeAll()    { mTriggerRandomizeAll    = true; }

    private:
      RegisterState mState;
      RegisterState mBuffer;

      bool mTriggerable            = false;
      bool mTriggerDeserialize     = false;
      bool mTriggerZeroWindow      = false;
      bool mTriggerZeroAll         = false;
      bool mTriggerRandomizeWindow = false;
      bool mTriggerRandomizeAll    = false;
  };
}