#pragma once

#include <od/objects/Object.h>
#include <od/config.h>
#include <sstream>
#include <vector>
#include <util.h>
#include <compressor.h>
#include <filter.h>

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace strike {
  class CPR : public od::Object {
    public:
      CPR() {
        addInput(mInLeft);
        addInput(mInRight);
        addInput(mSidechain);
        addOutput(mInLeftPGain);
        addOutput(mInRightPGain);
        addOutput(mOutLeft);
        addOutput(mOutRight);
        addOutput(mReduction);
        addOutput(mExcite);
        addOutput(mEOF);
        addOutput(mEOR);

        addParameter(mThreshold);
        addParameter(mRatio);
        addParameter(mMakeup);
        addParameter(mRise);
        addParameter(mFall);
        addParameter(mInputGain);
        addParameter(mOutputGain);

        addOption(mAutoMakeupGain);
        addOption(mEnableSidechain);
      }

      virtual ~CPR() { }

#ifndef SWIGLUA
      virtual void process();

      inline void processInternal() {
        const float *inLeft    = mInLeft.buffer();
        const float *inRight   = mInRight.buffer();
        const float *sidechain = mSidechain.buffer();

        float *inLeftPGain  = mInLeftPGain.buffer();
        float *inRightPGain = mInRightPGain.buffer();
        float *outLeft      = mOutLeft.buffer();
        float *outRight     = mOutRight.buffer();

        float *reduction = mReduction.buffer();
        float *excite    = mExcite.buffer();
        float *eof       = mEOF.buffer();
        float *eor       = mEOR.buffer();

        const auto threshold  = mThreshold.value();
        const auto ratio      = mRatio.value();
        const auto rise       = mRise.value();
        const auto fall       = mFall.value();
        const auto inputGain  = vdupq_n_f32(mInputGain.value());
        const auto outputGain = vdupq_n_f32(mOutputGain.value());

        const auto autoMakeupGain  = isAutoMakeupEnabled();
        const auto enableSidechain = isSidechainEnabled() ? vdupq_n_u32(0xffffffff) : vdupq_n_u32(0);

        mCompressor.setRiseFall(rise, fall);
        mCompressor.setThresholdRatio(threshold, ratio, autoMakeupGain);
        mMakeup.hardSet(mCompressor.mMakeupDb);

        auto zero = vdupq_n_f32(0);

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto _left      = vld1q_f32(inLeft + i) * inputGain;
          auto _right     = vld1q_f32(inRight + i) * inputGain;
          auto _sidechain = vld1q_f32(sidechain + i) * inputGain;

          auto _exciteLeft  = vbslq_f32(enableSidechain, _sidechain, _left);
          auto _exciteRight = vbslq_f32(enableSidechain, zero, _right);
          vst1q_f32(inLeftPGain + i, _exciteLeft);
          vst1q_f32(inRightPGain + i, _exciteRight);

          auto _exciteHigh   = vmaxq_f32(vmaxq_f32(_exciteLeft, _exciteRight), zero);
          auto _exciteLow    = vminq_f32(vminq_f32(_exciteLeft, _exciteRight), zero);
          auto _excite       = vmaxq_f32(_exciteHigh, vabsq_f32(_exciteLow));
          vst1q_f32(excite + i, _exciteHigh + _exciteLow);

          mCompressor.excite(_excite);
          vst1q_f32(reduction + i, mCompressor.mReduction);
          vst1q_f32(eof + i, vcvtq_n_f32_u32(mCompressor.mActive, 32));
          vst1q_f32(eor + i, vcvtq_n_f32_u32(vmvnq_u32(mCompressor.mActive), 32));

          auto _outputGain = mCompressor.makeup(outputGain);
          vst1q_f32(outLeft + i, mCompressor.compress(_left) * _outputGain);
          vst1q_f32(outRight + i, mCompressor.compress(_right) * _outputGain);
        }
      }

      od::Inlet  mInLeft       { "In1" };
      od::Inlet  mInRight      { "In2" };
      od::Inlet  mSidechain    { "Sidechain" };
      od::Outlet mInLeftPGain  { "Left In Post Gain" };
      od::Outlet mInRightPGain { "Right In Post Gain" };
      od::Outlet mOutLeft      { "Out1" };
      od::Outlet mOutRight     { "Out2" };

      od::Outlet mReduction { "Reduction" };
      od::Outlet mExcite    { "Excite" };
      od::Outlet mEOF       { "EOF" };
      od::Outlet mEOR       { "EOR" };

      od::Parameter mThreshold  { "Threshold", 0.5 };
      od::Parameter mRatio      { "Ratio", 2 };
      od::Parameter mMakeup     { "Makeup", 0 };
      od::Parameter mRise       { "Rise", 0.001 };
      od::Parameter mFall       { "Fall", 0.05 };
      od::Parameter mInputGain  { "Input Gain", 1 };
      od::Parameter mOutputGain { "Output Gain", 1 };

      od::Option mAutoMakeupGain  { "Auto Makeup Gain", 2 };
      od::Option mEnableSidechain { "Enable Sidechain", 2 };
#endif

      bool isAutoMakeupEnabled() {
        return mAutoMakeupGain.value() == 1;
      }

      void toggleAutoMakeup() {
        if (isAutoMakeupEnabled()) {
          mAutoMakeupGain.set(2);
        } else {
          mAutoMakeupGain.set(1);
        }
      }

      bool isSidechainEnabled() {
        return mEnableSidechain.value() == 1;
      }

      void toggleSidechainEnabled() {
        if (isSidechainEnabled()) {
          mEnableSidechain.set(2);
        } else {
          mEnableSidechain.set(1);
        }
      }

    private:
      compressor::Compressor mCompressor;
  };
}
