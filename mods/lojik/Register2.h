#pragma once

#include <od/objects/Object.h>
#include <od/extras/Random.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/simd.h>
#include <util/math.h>
#include <dsp/quantizer.h>
#include <graphics/interfaces/all.h>
#include <dsp/objects/CircularParameter.h>

#include <vector>

namespace lojik {
  #define REGISTER_MAX 128

  class Register2 : public od::Object, public graphics::HasChart, public graphics::HasScale, public graphics::HasScaleBook {
    public:
      Register2() {
        addInput(mIn);
        addOutput(mOut);
        addOutput(mQuantizeTrigger);

        addCircularParameter(mOffset);
        addCircularParameter(mShift);
        addCircularParameter(mLength);
        addCircularParameter(mStride);

        addParameter(mOutputGain);
        addParameter(mOutputBias);
        addParameter(mInputGain);
        addParameter(mInputBias);
        addParameter(mQuantizeScale);

        addInput(mClockTrigger);
        addInput(mCaptureGate);
        addInput(mShiftGate);
        addInput(mResetGate);
      }

      virtual ~Register2() { }

#ifndef SWIGLUA
      virtual void process();

      inline void processInternal() {
        updateCircularParameters();

        const float *in           = mIn.buffer();
        const float *clockTrigger = mClockTrigger.buffer();
        const float *captureGate  = mCaptureGate.buffer();
        const float *shiftGate    = mShiftGate.buffer();
        const float *resetGate    = mResetGate.buffer();

        mScaleQuantizer.setCurrent(mQuantizeScale.value());

        mState.setAbsoluteOffset(mOffset.value());
        mState.setWindowShift(mShift.value());
        mState.setWindowStride(mStride.value());
        mState.setWindowLength(mLength.value());

        if (mTriggerRandomizeWindow) {
          mTriggerRandomizeWindow = false;
          mState.randomizeWindow();
        }

        const auto outputGain = vdupq_n_f32(mOutputGain.value());
        const auto outputBias = vdupq_n_f32(mOutputBias.value());

        const auto inputGain  = vdupq_n_f32(mInputGain.value());
        const auto inputBias  = vdupq_n_f32(mInputBias.value());

        float *out = mOut.buffer();
        float *qTrig = mQuantizeTrigger.buffer();

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

          float tmp[4];
          for (int j = 0; j < 4; j++) {
            auto index = j * 4;
            auto jc    = vld1q_dup_u32(ccsr + index);
            auto jccsr = vld1q_u32(ccsr + index);

            auto _cTrig    = mClockTrig.read(jc);
            auto _ccsrTrig = mCSRTrigger.read(jccsr, jc);

            bool doStep    = vgetq_lane_u32(_cTrig, 0);
            bool doCapture = vgetq_lane_u32(_ccsrTrig, 1);
            bool doShift   = vgetq_lane_u32(_ccsrTrig, 2);
            bool doReset   = vgetq_lane_u32(_ccsrTrig, 3);

            bool doAdvance = doStep && !doShift;

            if (doAdvance) mState.advance();
            if (doShift)   mState.shift();
            if (doReset)   mState.reset();
            if (doCapture) mState.setCurrent(ingb[j]);

            tmp[j] = mState.windowValue();
          }

          auto _tmp = vld1q_f32(tmp);
          _tmp = _tmp * outputGain + outputBias;
          _tmp = mScaleQuantizer.process(_tmp);
          vst1q_f32(out + i, _tmp);
        }
      }

      od::Inlet  mIn  { "In" };
      od::Outlet mOut { "Out" };
      od::Outlet mQuantizeTrigger { "Quantize Trigger" };

      dsp::objects::CircularParameter mOffset { "Offset" };
      dsp::objects::CircularParameter mShift  { "Shift" };
      dsp::objects::CircularParameter mLength { "Length" };
      dsp::objects::CircularParameter mStride { "Stride" };

      od::Parameter mOutputGain { "Output Gain", 1.0f };
      od::Parameter mOutputBias { "Output Bias", 0.0f };
      od::Parameter mInputGain { "Input Gain", 1.0f };
      od::Parameter mInputBias { "Input Bias", 0.0f };
      od::Parameter mQuantizeScale { "Quantize Scale", 0 };

      od::Inlet mClockTrigger { "Clock" };
      od::Inlet mCaptureGate  { "Capture" };
      od::Inlet mShiftGate    { "Shift" };
      od::Inlet mResetGate    { "Reset" };
#endif

      void triggerRandomizeWindow() {
        mTriggerRandomizeWindow = true;
      }

      const common::Scale& currentScale() {
        return mScaleQuantizer.current();
      }

      int   getChartSize()         { return mState.windowLength(); }
      int   getChartCurrentIndex() { return mState.windowIndex(); }
      int   getChartBaseIndex()    { return mState.windowBaseIndex(); }
      float getChartValue(int i)   { return mState.windowValueAt(i); }

      std::string getScaleName()           { return currentScale().name(); }
      int         getScaleSize()           { return currentScale().size(); }
      float       getScaleCentValue(int i) { return currentScale().getCentValue(i); }
      float       getDetectedCentValue()   { return mScaleQuantizer.detectedCentValue(); }
      int         getDetectedOctaveValue() { return mScaleQuantizer.detectedOctaveValue(); }
      float       getQuantizedCentValue()  { return mScaleQuantizer.quantizedCentValue(); }

      int         getScaleBookIndex() { return mScaleQuantizer.getCurrentIndex(); }
      int         getScaleBookSize()  { return mScaleQuantizer.getScaleBookSize(); }
      std::string getScaleName(int i) { return mScaleQuantizer.getScale(i).name(); }
      int         getScaleSize(int i) { return mScaleQuantizer.getScale(i).size(); }

      void attach() { Object::attach(); }
      void release() { Object::release(); }

      dsp::objects::CircularParameter* getCircularParameter(const std::string &name) {
        for (auto *param : mCircularParameters) {
          if (name == param->name())
            return param;
        }

        return 0;
      }

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

          inline void randomizeWindow() {
            for (int i = 0; i < windowLength(); i++) {
              mData[toAbsolute(i)] = od::Random::generateFloat(-1, 1);
            }
          }

          inline void setAbsoluteOffset(int o) { mAbsoluteOffset = o; }
          inline void setWindowShift(int s)    { mWindowShift = s; }
          inline void setWindowStride(int s)   { mWindowStride = s; }
          inline void setWindowLength(int l)   { mWindowLength = l; }

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
          inline int   windowBaseIndex()    const { return util::mod(mWindowIndex - mWindowShift, mWindowLength); }
          inline float windowValueAt(int i) const { return valueAt(toAbsolute(i)); }
          inline float windowValue()        const { return windowValueAt(mWindowIndex); }
          inline int   absoluteIndex()      const { return toAbsolute(windowIndex()); }

        private:
          inline int toAbsolute(int index) const {
            auto window = util::mod(mWindowShift + index, mWindowLength);
            return util::mod(mAbsoluteOffset + window, max);
          }

          inline float valueAt(int i) const {
            return mData[i];
          }

          float mData[max];
          int mAbsoluteOffset = 0;
          int mWindowIndex    = 0;
          int mWindowShift    = 0;
          int mWindowStride   = 1;
          int mWindowLength   = max;
      };

      bool mTriggerRandomizeWindow = false;

      common::ScaleQuantizer mScaleQuantizer;

      util::four::Trigger mClockTrig;
      util::four::SyncTrigger mCSRTrigger;
      State<REGISTER_MAX> mState;

      void addCircularParameter(dsp::objects::CircularParameter &parameter) {
        mCircularParameters.push_back(&parameter);
        own(parameter);
      }

      void updateCircularParameters() {
        for (auto *param : mCircularParameters) {
          param->update();
        }
      }

      std::vector<dsp::objects::CircularParameter *> mCircularParameters;
  };
}