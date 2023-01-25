#pragma once

#include <od/objects/heads/TapeHead.h>
#include <od/audio/Sample.h>
#include <od/audio/Slices.h>
#include <dsp/latch.h>
#include <dsp/tempo.h>
#include <dsp/slew.h>
#include <sstream>
#include <util/math.h>

namespace sloop {
  #define JUMP_MODE_NORMAL 1
  #define JUMP_MODE_STEP 2
  #define JUMP_MODE_RANDOM 3

  #define PLAY_MODE_NORMAL 1
  #define PLAY_MODE_REVERSE 2
  #define PLAY_MODE_PINGPONG 3

  class Sloop2 : public od::TapeHead {
    public:
      Sloop2(bool stereo) {
        mStereo = stereo;

        for (int channel = 0; channel < channels(); channel++) {
          std::ostringstream inName;
          inName << "In" << channel + 1;
          addInputFromHeap(new od::Inlet { inName.str() });

          std::ostringstream outName;
          outName << "Out" << channel + 1;
          addOutputFromHeap(new od::Outlet { outName.str() });
        }

        addInput(mTap);
        addInput(mEngage);
        addInput(mWrite);
        addInput(mJump);
        addInput(mReverse);

        addParameter(mMaxLength);
        addParameter(mLength);
        addParameter(mThrough);
        addParameter(mFeedback);
        addParameter(mFade);
        addParameter(mJumpValue);

        addOption(mJumpMode);
        addOption(mPlayMode);
      }

      virtual ~Sloop2() {
        if (mpSlices) mpSlices->release();
      }

#ifndef SWIGLUA
      virtual void process();

      od::Inlet mTap       { "Tap" };
      od::Inlet mEngage    { "Engage" };
      od::Inlet mWrite     { "Write" };
      od::Inlet mJump      { "Jump" };
      od::Inlet mReverse   { "Reverse" };

      od::Outlet mClockOut { "Clock Out" };
      od::Outlet mJumpOut  { "Jump Out" };

      od::Parameter mMaxLength { "Length", 4 };
      od::Parameter mLength    { "Length", 4 };
      od::Parameter mThrough   { "Through", 1.0 };
      od::Parameter mFeedback  { "Feedback", 1.0 };
      od::Parameter mFade      { "Fade", 0.005 };
      od::Parameter mJumpValue { "Jump Value", 0 };

      od::Option mJumpMode { "Jump Mode", JUMP_MODE_NORMAL };
      od::Option mPlayMode { "Play Mode", PLAY_MODE_NORMAL };
#endif

    void setSample(od::Sample *sample) {
      setSample(sample, NULL);
    }

    void setSample(od::Sample *sample, od::Slices *slices) {
      od::TapeHead::setSample(sample);
      setSlices(slices);
    }

    void setSlices(od::Slices *slices) {
      od::Slices *pSlices = mpSlices;
      mpSlices = 0;
      if (pSlices) pSlices->release();

      pSlices = slices;
      if (pSlices) pSlices->attach();
      mpSlices = pSlices;
    }

    int getInternalClockPeriod() {
      return mInternalClock.mPeriod;
    }

    int getSampleLength() {
      return mEndIndex;
    }

    bool isJumpPending() {
      return vgetq_lane_u32(mLatches.state(), 2);
    }

    int jumpPosition() {
      return mJumpValue.value() * getInternalClockPeriod();
    }

    int lastPosition() {
      return mLength.value() * getInternalClockPeriod();
    }

    float writeLevel() {
      return vgetq_lane_f32(mSlews.mValue, 0);
    }

    enum State {
      Hold,
      Write,
      PlayReverse,
      PlayForward
    };

    State state() {
      auto latch = mLatches.state();
      if (!vgetq_lane_u32(latch, 0)) return Hold;
      if (vgetq_lane_u32(latch, 1)) return Write;
      if (vgetq_lane_u32(latch, 3)) return PlayReverse;
      return PlayForward;
    }

    private:
      od::Slices *mpSlices = 0;

      /**
        * Are we outputing in stereo?
        */
      bool mStereo = false;
      inline int channels() const { return mStereo ? 2 : 1; }

      dsp::TapTempo mInternalClock;
      dsp::four::SyncedCountLatch mLatches;
      slew::four::Slew mSlews;

      int mPlayDirection = 1;

      inline void processInternal() {
        if (mpSample == 0) return;

        Buffers   buffers { *this };
        Constants constants { *this };
        mSlews.setRiseFall(constants.fade, constants.fade);

        if (channels() < 2) {
          if (mpSample->mChannelCount < 2) {
            // Mono unit + mono sample
            for (int i = 0; i < FRAMELENGTH; i += 4) {
              auto instant = processInstant(buffers, constants, i);
              instant.processMonoToMono();
            }
          } else {
            // Mono unit + stereo sample
            for (int i = 0; i < FRAMELENGTH; i += 4) {
              auto instant = processInstant(buffers, constants, i);
              instant.processMonoToStereo();
            }
          }
        } else {
          if (mpSample->mChannelCount < 2) {
            // Stereo unit + mono sample
            for (int i = 0; i < FRAMELENGTH; i += 4) {
              auto instant = processInstant(buffers, constants, i);
              instant.processStereoToMono();
            }
          } else {
            // Stereo unit + stereo sample
            for (int i = 0; i < FRAMELENGTH; i += 4) {
              auto instant = processInstant(buffers, constants, i);
              instant.processStereoToStereo();
            }
          }
        }
      }

      struct Buffers {
        float *mIn[2], *mTap, *mEngage, *mWrite, *mJump, *mReverse;
        float *mOut[2], *mClockOut, *mJumpOut;

        inline Buffers(Sloop2 &self) {
          for (int channel = 0; channel < self.channels(); channel++) {
            mIn[channel] = self.getInput(channel)->buffer();
            mOut[channel] = self.getOutput(channel)->buffer();
          }

          mTap     = self.mTap.buffer();
          mEngage  = self.mEngage.buffer();
          mWrite   = self.mWrite.buffer();
          mJump    = self.mJump.buffer();
          mReverse = self.mReverse.buffer();

          mClockOut = self.mClockOut.buffer();
          mJumpOut  = self.mJumpOut.buffer();
        }

        inline float32x4_t in(int channel, int offset) const {
          return vld1q_f32(mIn[channel]  + offset);
        }

        inline void out(int channel, int offset, float32x4_t value) {
          vst1q_f32(mOut[channel] + offset, value);
        }

        inline void clockOut(int offset, float32x4_t value) { vst1q_f32(mClockOut + offset, value); }
        inline void jumpOut(int offset, float32x4_t value) { vst1q_f32(mJumpOut + offset, value); }

        inline float32x4_t tap(int offset)     const { return vld1q_f32(mTap + offset); }
        inline float32x4_t engage(int offset)  const { return vld1q_f32(mEngage + offset); }
        inline float32x4_t write(int offset)   const { return vld1q_f32(mWrite + offset); }
        inline float32x4_t jump(int offset)    const { return vld1q_f32(mJump + offset); }
        inline float32x4_t reverse(int offset) const { return vld1q_f32(mReverse + offset); }
      };

      struct Constants {
        float through, feedback;
        int maxLength, length;
        int jumpValue, jumpMode;
        float32x4_t fade;

        inline Constants(Sloop2 &self) {
          maxLength = self.mMaxLength.value();
          length    = self.mLength.value();
          through   = self.mThrough.value();
          feedback  = self.mFeedback.value();
          jumpValue = self.mJumpValue.value();
          jumpMode  = self.mJumpMode.value();
          fade      = vdupq_n_f32(self.mFade.value());
        }
      };

      struct StepLevels {
        float write, shadow;
      };

      struct StepInputs {
        uint32_t tapTrigger;
        uint32_t engageGate;
        uint32_t writeGate;
        uint32_t jumpGate;
        uint32_t reverseGate;

        uint32_t jumpTrigger() const { return jumpGate & tapTrigger; }
        uint32_t reverseTrigger() const { return reverseGate & tapTrigger; }

        StepLevels processLevels(slew::four::Slew &slew) {
          auto targets = util::four::make_u(writeGate, jumpTrigger(), 0, 0);
          auto values = slew.process(vcvtq_n_f32_u32(targets, 32));

          return StepLevels {
            vgetq_lane_f32(values, 0),
            vgetq_lane_f32(values, 1)
          };
        }
      };

      struct InputVectors {
        uint32_t tap[4], engage[4], write[4], jump[4], reverse[4];

        inline InputVectors(const Buffers& buffers, int offset) {
          vst1q_u32(tap,     vcgtq_f32(buffers.tap(offset),     vdupq_n_f32(0)));
          vst1q_u32(engage,  vcgtq_f32(buffers.engage(offset),  vdupq_n_f32(0)));
          vst1q_u32(write,   vcgtq_f32(buffers.write(offset),   vdupq_n_f32(0)));
          vst1q_u32(jump,    vcgtq_f32(buffers.jump(offset),    vdupq_n_f32(0)));
          vst1q_u32(reverse, vcgtq_f32(buffers.reverse(offset), vdupq_n_f32(0)));
        }

        uint32x4_t ewjr(int step) {
          return util::four::make_u(engage[step], write[step], jump[step], reverse[step]);
        }

        StepInputs processLatch(dsp::four::SyncedCountLatch &latch, uint32_t sync, int step) {
          auto signal = util::four::make_u(engage[step], write[step], jump[step], reverse[step]);
          auto output = latch.process(signal, vdupq_n_u32(sync));

          return StepInputs {
            sync,
            vgetq_lane_u32(output, 0),
            vgetq_lane_u32(output, 1),
            vgetq_lane_u32(output, 2),
            vgetq_lane_u32(output, 3),
          };
        }
      };

      struct Step {
        int sampleIndex, shadowIndex;
        float writeLevel, shadowLevel;
      };

      struct Instant {
        int mOffset;

        Buffers &mBuffers;
        od::Sample *mpSample;
        int mSampleIndex[4], mShadowIndex[4];
        util::simd::Complement mWriteLevel, mShadowLevel;
        float32x4_t mFeedbackLevel, mThroughLevel;

        Instant(Buffers &buffers, Step (&steps)[4], od::Sample *pSample, int offset)
         : mBuffers(buffers), mpSample(pSample), mOffset(offset) {

          float _writeLevel[4], _shadowLevel[4];
          for (int i = 0; i < 4; i++) {
            mSampleIndex[i] = steps[i].sampleIndex;
            mShadowIndex[i] = steps[i].shadowIndex;
            _writeLevel[i] = steps[i].writeLevel;
            _shadowLevel[i] = steps[i].shadowLevel;
          }

          mWriteLevel = util::simd::Complement { vld1q_f32(_writeLevel) };
          mShadowLevel = util::simd::Complement { vld1q_f32(_shadowLevel) };
          mFeedbackLevel = vdupq_n_f32(0);
          mThroughLevel = vdupq_n_f32(1);
        }

        float32x4_t in(int channel) { return mBuffers.in(channel, mOffset); }
        void out(int channel, float32x4_t value) { mBuffers.out(channel, mOffset, value); }

        inline float32x4_t processSample(float32x4_t input, int channel) {
          float32x4_t original = util::simd::readSample(mpSample, mSampleIndex, channel);

          float32x4_t update;
          update = vmlaq_f32(input, original, mFeedbackLevel);
          update = mWriteLevel.lerp(update, original);
          update = mShadowLevel.lerp(original, update);

          util::simd::writeSample(mpSample, mSampleIndex, channel, update);
          return update;
        }

        inline float32x4_t processShadow(float32x4_t input, int channel) {
          float32x4_t original = util::simd::readSample(mpSample, mShadowIndex, channel);

          float32x4_t update;
          update = vmlaq_f32(input, original, mFeedbackLevel);
          update = mWriteLevel.lerp(update, original);
          update = mShadowLevel.lerp(update, original);

          util::simd::writeSample(mpSample, mShadowIndex, channel, update);
          return update;
        }

        void processMonoToMono() {
          float32x4_t input = in(0);

          float32x4_t current = processSample(input, 0);
          float32x4_t shadow  = processShadow(input, 0);
          float32x4_t mix = mShadowLevel.lerp(shadow, current);

          out(0, vmlaq_f32(mix, input, mThroughLevel));
        }

        inline void processMonoToStereo() {
          float32x4_t input = in(0);

          float32x4_t lCurrent = processSample(input, 0);
          float32x4_t rCurrent = processSample(input, 1);
          float32x4_t lShadow  = processShadow(input, 0);
          float32x4_t rShadow  = processShadow(input, 1);

          float32x4_t left  = mShadowLevel.lerp(lShadow, lCurrent);
          float32x4_t right = mShadowLevel.lerp(rShadow, rCurrent);
          float32x4_t mix   = vdupq_n_f32(0.5) * (left + right);

          out(0, vmlaq_f32(mix, input, mThroughLevel));
        }

        inline void processStereoToMono() {
          float32x4_t lInput = in(0);
          float32x4_t rInput = in(1);
          float32x4_t input  = vdupq_n_f32(0.5) * (lInput + rInput);

          float32x4_t current = processSample(input, 0);
          float32x4_t shadow  = processShadow(input, 0);
          float32x4_t mix = mShadowLevel.lerp(shadow, current);

          out(0, vmlaq_f32(mix, lInput, mThroughLevel));
          out(1, vmlaq_f32(mix, rInput, mThroughLevel));
        }

        inline void processStereoToStereo() {
          float32x4_t lInput = in(0);
          float32x4_t rInput = in(1);

          float32x4_t lCurrent = processSample(lInput, 0);
          float32x4_t rCurrent = processSample(rInput, 1);
          float32x4_t lShadow  = processShadow(lInput, 0);
          float32x4_t rShadow  = processShadow(rInput, 1);

          float32x4_t lMix = mShadowLevel.lerp(lShadow, lCurrent);
          float32x4_t rMix = mShadowLevel.lerp(rShadow, rCurrent);

          out(0, vmlaq_f32(lMix, lInput, mThroughLevel));
          out(1, vmlaq_f32(rMix, rInput, mThroughLevel));
        }
      };

      inline Step processStep(InputVectors &vectors, int stepIndex) {
        Step step;

        mInternalClock.process(vectors.tap[stepIndex]);
        uint32_t sync = util::bcvt((mCurrentIndex % mInternalClock.mPeriod) == 0);

        auto stepInputs = vectors.processLatch(mLatches, sync, stepIndex);
        auto stepLevels = stepInputs.processLevels(mSlews);

        step.sampleIndex = mCurrentIndex;
        step.shadowIndex = mShadowIndex;
        step.writeLevel  = stepLevels.write;
        step.shadowLevel = stepLevels.shadow;

        if (stepInputs.reverseTrigger()) mPlayDirection = -mPlayDirection;
        auto increment = stepInputs.engageGate & mPlayDirection;
        mCurrentIndex += increment;
        mShadowIndex += increment;

        if (stepInputs.jumpTrigger()) {
          mShadowIndex = mCurrentIndex;
          mCurrentIndex = 0;
        } else {
          if (mCurrentIndex >= mEndIndex) mCurrentIndex = 0;
          if (mCurrentIndex < 0) mCurrentIndex = mEndIndex - 1;

          if (mShadowIndex  >= mEndIndex) mShadowIndex = 0;
          if (mShadowIndex < 0) mShadowIndex = mEndIndex - 1;
        }

        if (stepInputs.writeGate) mpSample->mDirty = true;

        return step;
      }

      inline Instant processInstant(Buffers& buffers, Constants& constants, int offset) {
        InputVectors vectors { buffers, offset };

        Step steps[4];
        for (int i = 0; i < 4; i++) {
          steps[i] = processStep(vectors, i);
        }

        return Instant { buffers, steps, mpSample, offset };
      }
  };
}