#pragma once

#include <util.h>
#include <od/extras/Random.h>
#include <vector>

#define REGISTER_MAX 1024

namespace lojik {

  template <int max>
  struct RegisterState2 {
    inline void reset() { mCurrent = 0; }

    float mData[max];
    float mDrift[max];

    int mCurrent = 0;
    int mShift   = 0;
    int mStride  = 1;
    int mTotal   = max;

    float mDriftAmount = 0.0f;

  };

  class RegisterState {
    public:
      int   max()       { return mData.size(); }
      int   limit()     { return mLimit; }
      int   step()      { return mStep; }
      int   shift()     { return mShift; }
      float data(int i) { return mData[i] + mDrifts[i]; }
      int   current()   { return index(mStep, mShift); }

      void setMax(uint32_t v)  { mData.resize(util::clamp(v, 1, REGISTER_MAX), 0.0f); mDrifts.resize(mData.size()); }
      void setStep(int v)      { mStep    = v; }
      void setShift(int v)     { mShift   = v; }
      void setLimit(int l)     { mLimit   = util::clamp(l, 1, max()); }
      void setStride(int s)    { mStride  = s; }
      void setScatter(float s) { mScatter = s; }
      void setDrift(float d)   { mDrift   = util::fclamp(d, 0.0f, 10.f); }
      void setGain(float g)    { mGain    = g; }
      void setBias(float b)    { mBias    = b; }

      void setData(int i, float v) { set(i, v, mGain, mBias, mScatter); }

      void doStep()  { mStep  = index(mStep, mStride); }
      void doShift() { mShift = index(mShift, mStride); }
      void doDrift() { drift(current()); }
      void doReset() { mStep  = 0; }

      void markZeroWindow()      { mZeroWindow      = true; }
      void markScatterWindow()   { mScatterWindow   = true; }
      void markRandomizeWindow() { mRandomizeWindow = true; }

      void markZeroAll()         { mZeroAll         = true; }
      void markScatterAll()      { mScatterAll      = true; }
      void markRandomizeAll()    { mRandomizeAll    = true; }

      void processTriggers() {
        if (mZeroWindow)      { setWindow(&RegisterState::zero);      mZeroWindow      = false; }
        if (mScatterWindow)   { setWindow(&RegisterState::scatter);   mScatterWindow   = false; }
        if (mRandomizeWindow) { setWindow(&RegisterState::randomize); mRandomizeWindow = false; }

        if (mZeroAll)      { setAll(&RegisterState::zero);      mZeroAll      = false; }
        if (mScatterAll)   { setAll(&RegisterState::scatter);   mScatterAll   = false; }
        if (mRandomizeAll) { setAll(&RegisterState::randomize); mRandomizeAll = false; }
      }

    private:
      std::vector<float> mData;
      std::vector<float> mDrifts;
      int   mStep   = 0;
      int   mShift  = 0;
      int   mStride = 0;
      int   mLimit  = 1;

      float mScatter = 0.0f;
      float mDrift   = 0.0f;
      float mGain    = 1.0f;
      float mBias    = 0.0f;

      bool mZeroWindow      = false;
      bool mScatterWindow   = false;
      bool mRandomizeWindow = false;

      bool mZeroAll         = false;
      bool mScatterAll      = false;
      bool mRandomizeAll    = false;

      void set(int index, float value, float gain, float bias, float scatter) {
        float input = bias + value * gain;
        float noise = scatter * od::Random::generateFloat(-1.0f, 1.0f);
        mData[index] = input + noise;
      }

      void zero(int i)      { set(i, 0, 0, 0, 0); }
      void scatter(int i)   { set(i, 0, 0, data(i), mScatter); }
      void randomize(int i) { set(i, 0, 0, 0, mScatter); }

      void drift(int i) {
        float scale = 0.1f;
        float delta = od::Random::generateFloat(
          -1 + -0.5f * (mDrifts[i] - 1.0f),
           1 -  0.5f * (mDrifts[i] + 1.0f)
        );
        mDrifts[i] = (mDrifts[i] + scale * delta) * mDrift;
      }

      int index(int base, int offset) {
        return util::mod(base + offset, mLimit);
      }

      void setWindow(void (RegisterState::*f)(int)) {
        int start   = current();
        int current = start;
        do {
          (this->*f)(current);
          current = this->index(current, mStride);
        } while (start != current);
      }

      void setAll(void (RegisterState::*f)(int)) {
        for (int i = 0; i < max(); i++) (this->*f)(i);
      }
  };
}