#pragma once

#include <od/objects/Object.h>
#include <od/extras/Random.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/simd.h>
#include <util.h>
#include <ScaleQuantizer.h>
#include <HasChartData.h>
#include <HasScaleData.h>

namespace lojik {
  class Register2 : public od::Object, public common::HasChartData, public common::HasScaleData {
    public:
      Register2() {
        addInput(mIn);
        addOutput(mOut);

        addParameter(mOffset);
        addParameter(mShift);
        addParameter(mLength);
        addParameter(mStride);

        addParameter(mOutputGain);
        addParameter(mOutputBias);
        addParameter(mInputGain);
        addParameter(mInputBias);
        addParameter(mQuantizeScale);

        addInput(mClockTrigger);
        addInput(mCaptureGate);
        addInput(mShiftGate);
        addInput(mResetGate);

        addOption(mSync);
        addOption(mQuantize);
      }

      virtual ~Register2() { }

#ifndef SWIGLUA
      virtual void process();

      inline void processInternal() {
        const float *in           = mIn.buffer();
        const float *clockTrigger = mClockTrigger.buffer();
        const float *captureGate  = mCaptureGate.buffer();
        const float *shiftGate    = mShiftGate.buffer();
        const float *resetGate    = mResetGate.buffer();

        mState.setAbsoluteOffset(mOffset.value());
        mState.setWindowShift(mShift.value());
        mState.setWindowStride(mStride.value());
        mState.setWindowLength(mLength.value());

        mState.setOutputGain(mOutputGain.value());
        mState.setOutputBias(mOutputBias.value());

        const auto inputGain  = vdupq_n_f32(mInputGain.value());
        const auto inputBias  = vdupq_n_f32(mInputBias.value());
        const auto quantize   = isQuantized();

        const auto sync = vmvnq_u32(util::four::make_u32(
          false,
          mSync.getFlag(0),
          mSync.getFlag(1),
          mSync.getFlag(2)
        ));

        float *out = mOut.buffer();

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto _in = vld1q_f32(in + i) * inputGain + inputBias;
          float ingb[4];
          vst1q_f32(ingb, _in);

          auto zero          = vdupq_n_f32(0);
          auto _clockTrigger = vcgtq_f32(vld1q_f32(clockTrigger + i), zero);
          auto _captureGate  = vcgtq_f32(vld1q_f32(captureGate + i), zero);
          auto _shiftGate    = vcgtq_f32(vld1q_f32(shiftGate + i), zero);
          auto _resetGate    = vcgtq_f32(vld1q_f32(resetGate + i), zero);

          uint32x4x4_t _ccsr { _clockTrigger, _captureGate, _shiftGate, _resetGate };
          uint32_t ccsr[16];
          vst4q_u32(ccsr, _ccsr);

          for (int j = 0; j < 4; j++) {
            auto index = j * 4;
            auto jc    = vld1q_dup_u32(ccsr + index);
            auto jccsr = vld1q_u32(ccsr + index);

            auto _cTrig    = mClockTrig.read(jc);
            auto _ccsrTrig = mCSRTrigger.read(jccsr, sync | jc);

            bool doStep    = vgetq_lane_u32(_cTrig, 0);
            bool doCapture = vgetq_lane_u32(_ccsrTrig, 1);
            bool doShift   = vgetq_lane_u32(_ccsrTrig, 2);
            bool doReset   = vgetq_lane_u32(_ccsrTrig, 3);

            bool doAdvance = doStep && !doShift;

            if (doAdvance) mState.advance();
            if (doShift)   mState.shift();
            if (doReset)   mState.reset();
            if (doCapture) mState.setCurrent(ingb[j]);

            auto value = mState.windowValue();
            value = mScaleQuantizer.quantizeAndDetect(value);
            out[i + j] = value;
          }
        }
      }

      od::Inlet  mIn  { "In" };
      od::Outlet mOut { "Out" };

      od::Parameter mOffset { "Offset" };
      od::Parameter mShift  { "Shift" };
      od::Parameter mLength { "Length" };
      od::Parameter mStride { "Stride" };

      od::Parameter mOutputGain { "Output Gain", 1.0f };
      od::Parameter mOutputBias { "Output Bias", 0.0f };
      od::Parameter mInputGain { "Input Gain", 1.0f };
      od::Parameter mInputBias { "Input Bias", 0.0f };
      od::Parameter mQuantizeScale { "Quantize Scale", 0 };

      od::Inlet mClockTrigger { "Clock" };
      od::Inlet mCaptureGate  { "Capture" };
      od::Inlet mShiftGate    { "Shift" };
      od::Inlet mResetGate    { "Reset" };

      od::Option mSync     { "Sync", 0b111 };
      od::Option mQuantize { "Quantize", 2 };
#endif

      int getChartSize() {
        return mState.windowLength();
      }

      int getChartCurrentIndex() {
        return mState.windowIndex();
      }

      float getChartValue(int i) {
        return mScaleQuantizer.quantize(mState.windowValueAt(i));
      }

      const common::Scale& currentScale() {
        return mScaleQuantizer.currentScale();
      }

      int getScaleSize() { return currentScale().size(); }
      float getScaleCentValue(int i) { return currentScale().getCentValue(i); }

      float getDetectedCentValue() { return mScaleQuantizer.detectedCentValue(); }
      float getQuantizedCentValue() { return mScaleQuantizer.quantizedCentValue(); }

      void attach() { Object::attach(); }
      void release() { Object::release(); }

      bool isQuantized() { return mQuantize.value() == 1; }

    private:
      template<int max>
      class State {
        public:
          inline State() {
            randomize();
          }

          inline void randomize() {
            for (int i = 0; i < max; i++) {
              mData[i] = od::Random::generateFloat(-1, 1);
            }
          }

          inline void setAbsoluteOffset(int o) { mAbsoluteOffset = o; }
          inline void setWindowShift(int s)    { mWindowShift = s; }
          inline void setWindowStride(int s)   { mWindowStride = s; }
          inline void setWindowLength(int l)   { mWindowLength = l; }

          inline void setOutputGain(float g) { mOutputGain = g; }
          inline void setOutputBias(float b) { mOutputBias = b; }

          inline void advance() {
            mWindowIndex = util::mod(mWindowIndex + mWindowStride, mWindowLength);
          }

          inline void shift() {
            mWindowShift = util::mod(mWindowShift + 1, mWindowLength);
          }

          inline void reset() {
            mWindowIndex = 0;
          }

          inline void setCurrent(float v) {
            mData[absoluteIndex()] = v;
          }

          inline int   windowLength()       const { return mWindowLength; }
          inline int   windowIndex()        const { return mWindowIndex; }
          inline float windowValueAt(int i) const { return valueAt(toAbsolute(i)); }
          inline float windowValue()        const { return windowValueAt(mWindowIndex); }
          inline int   absoluteIndex()      const { return toAbsolute(windowIndex()); }

        private:
          inline int toAbsolute(int index) const {
            auto window = util::mod(mWindowShift + index, mWindowLength);
            return util::mod(mAbsoluteOffset + window, max);
          }

          inline float valueAt(int i) const {
            return mData[i] * mOutputGain + mOutputBias;
          }

          float mData[max];
          int mAbsoluteOffset = 0;
          int mWindowIndex    = 0;
          int mWindowShift    = 0;
          int mWindowStride   = 1;
          int mWindowLength   = max;

          float mOutputGain = 1;
          float mOutputBias = 0;
      };

      common::ScaleQuantizer mScaleQuantizer;

      util::four::Trigger mClockTrig;
      util::four::SyncTrigger mCSRTrigger;
      State<128> mState;

  };
}