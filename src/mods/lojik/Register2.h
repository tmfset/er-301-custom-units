#pragma once

#include <od/objects/Object.h>
#include <od/extras/Random.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/simd.h>
#include <util.h>
#include <ScaleBook.h>
#include <HasChartData.h>

namespace lojik {
  class Register2 : public od::Object, public common::HasChartData {
    public:
      Register2() {
        addInput(mIn);
        addOutput(mOut);

        addParameter(mLength);
        addParameter(mStride);

        addParameter(mScatter);
        addParameter(mDrift);
        addParameter(mInputGain);
        addParameter(mInputBias);
        addParameter(mQuantizeScale);

        addInput(mClock);
        addInput(mCapture);
        addInput(mShift);
        addInput(mReset);

        addOption(mSync);
        addOption(mQuantize);
      }

      virtual ~Register2() { }

#ifndef SWIGLUA
      virtual void process();

      inline void processInternal() {
        const float *in      = mIn.buffer();
        const float *clock   = mClock.buffer();
        const float *capture = mCapture.buffer();
        const float *shift   = mShift.buffer();
        const float *reset   = mReset.buffer();

        mState.setTotal(mLength.value());
        const float stride = mStride.value();

        const auto scatter  = mScatter.value();
        const auto drift    = mDrift.value();
        const auto gain     = vdupq_n_f32(mInputGain.value());
        const auto bias     = vdupq_n_f32(mInputBias.value());
        const auto quantize = isQuantized();

        const auto sync = vmvnq_u32(util::four::make_u32(
          false,
          mSync.getFlag(0),
          mSync.getFlag(1),
          mSync.getFlag(2)
        ));

        float *out = mOut.buffer();

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto _in = bias + vld1q_f32(in + i) * gain;
          float ingb[4];
          vst1q_f32(ingb, _in);

          auto zero     = vdupq_n_f32(0);
          auto _clock   = vcgtq_f32(vld1q_f32(clock + i), zero);
          auto _capture = vcgtq_f32(vld1q_f32(capture + i), zero);
          auto _shift   = vcgtq_f32(vld1q_f32(shift + i), zero);
          auto _reset   = vcgtq_f32(vld1q_f32(reset + i), zero);

          uint32x4x4_t _ccsr { _clock, _capture, _shift, _reset };
          uint32_t ccsr[16];
          vst4q_u32(ccsr, _ccsr);

          for (int j = 0; j < 4; j++) {
            auto index = j * 4;
            auto jc    = vld1q_dup_u32(ccsr + index);
            auto jccsr = vld1q_u32(ccsr + index);

            auto _cTrig    = mClockTrigger.read(jc);
            auto _ccsrTrig = mCSRTrigger.read(jccsr, sync | jc);

            bool doStep    = vgetq_lane_u32(_cTrig, 0);
            bool doCapture = vgetq_lane_u32(_ccsrTrig, 1);
            bool doShift   = vgetq_lane_u32(_ccsrTrig, 2);
            bool doReset   = vgetq_lane_u32(_ccsrTrig, 3);

            bool doAdvance = doStep && !doShift;
            bool doDrift   = doStep || doShift || doReset;

            if (doAdvance) mState.advance();
            if (doShift)   mState.shift();
            if (doReset)   mState.reset();
            if (doCapture) mState.setCurrent(ingb[j]);

            out[i + j] = mState.value();
          }
        }
      }

      od::Inlet  mIn  { "In" };
      od::Outlet mOut { "Out" };

      od::Parameter mLength { "Length" };
      od::Parameter mStride { "Stride" };

      od::Parameter mScatter   { "Scatter",    0.0f };
      od::Parameter mDrift     { "Drift",      0.0f };
      od::Parameter mInputGain { "Input Gain", 1.0f };
      od::Parameter mInputBias { "Input Bias", 0.0f };
      od::Parameter mQuantizeScale { "Quantize Scale", 0 };

      od::Inlet mClock   { "Clock" };
      od::Inlet mCapture { "Capture" };
      od::Inlet mShift   { "Shift" };
      od::Inlet mReset   { "Reset" };

      od::Option mSync     { "Sync", 0b111 };
      od::Option mQuantize { "Quantize", 2 };
#endif

      int length() { return mState.mTotal; }
      int current() { return mState.mCurrent; }
      float value(int i) { return mState.valueAt(i); }

      void attach() { Object::attach(); }
      void release() { Object::release(); }

      bool isQuantized() { return mQuantize.value() == 1; }

      const common::Scale& currentScale() {
        return mScaleBook.getScale(
          util::fhr(mQuantizeScale.value()) % mScaleBook.size()
        );
      }

    private:
      template<int max>
      struct State {
        inline State() {
          for (int i = 0; i < max; i++) {
            mData[i]  = od::Random::generateFloat(-1.0f, 1.0f);
            mDrift[i] = 0;
          }
        }

        inline int current() { return index(mCurrent, mShift); }
        inline float value() { return valueAt(current()); }

        inline void advance() { mCurrent = index(mCurrent, mStride); }
        inline void shift()   { mShift   = index(mShift, mStride); }
        inline void reset()   { mCurrent = 0; }
        inline void setCurrent(float v) { mData[mCurrent] = v; }

        inline void setTotal(int t) { mTotal = util::clamp(t, 1, max); }

        inline float valueAt(int i) {
          return mData[i] + mDrift[i];
        }

        inline int index(int b, int o) {
          return util::mod(b + o, mTotal);
        }

        float mData[max];
        float mDrift[max];

        int mCurrent = 0;
        int mShift   = 0;
        int mStride  = 1;
        int mTotal   = max;
      };

      common::ScaleBook mScaleBook;

      util::four::Trigger mClockTrigger;
      util::four::SyncTrigger mCSRTrigger;
      State<128> mState;

  };
}