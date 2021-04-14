#pragma once

#include <od/objects/heads/TapeHead.h>
#include <od/extras/LinearRamp.h>
#include <od/audio/Slice.h>
#include <od/audio/Slices.h>
#include <od/audio/Sample.h>
#include <SyncLatch.h>
#include <Slew.h>
#include <sstream>
#include <vector>
#include <util.h>

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace sloop {
  class Sloop : public od::TapeHead {
    public:
      Sloop(od::Parameter *pLength, bool stereo) {
        mpProxyLength = pLength;
        mStereo = stereo;

        for (int channel = 0; channel < channels(); channel++) {
          std::ostringstream inName;
          inName << "In" << channel + 1;
          addInputFromHeap(new od::Inlet { inName.str() });

          std::ostringstream outName;
          outName << "Out" << channel + 1;
          addOutputFromHeap(new od::Outlet { outName.str() });
        }

        addInput(mClock);
        addInput(mEngage);
        addInput(mRecord);
        addInput(mOverdub);
        addInput(mReset);
        addInput(mLength);
        addInput(mRLength);

        addParameter(mThrough);
        addParameter(mFeedback);
        addParameter(mFade);
        addParameter(mFadeIn);
        addParameter(mFadeOut);
        addParameter(mResetTo);

        addOption(mResetFlags);
      }
      virtual ~Sloop() { }

      void setSample(od::Sample *sample) {
        setSample(sample, NULL);
      }

      void setSample(od::Sample *sample, od::Slices *slices) {
        od::TapeHead::setSample(sample);

        od::Slices *pSlices = mpSlices;
        mpSlices = 0;
        if (pSlices) pSlices->release();

        pSlices = slices;
        if (pSlices) pSlices->attach();
        mpSlices = pSlices;
      }

#ifndef SWIGLUA
      virtual void process();

      od::Inlet mClock   { "Clock" };
      od::Inlet mEngage  { "Engage" };
      od::Inlet mRecord  { "Record" };
      od::Inlet mOverdub { "Overdub" };
      od::Inlet mReset   { "Reset" };
      od::Inlet mLength  { "Length" };
      od::Inlet mRLength { "Record Length" };

      od::Parameter mThrough  { "Through" };
      od::Parameter mFeedback { "Feedback" };
      od::Parameter mFade     { "Fade" };
      od::Parameter mFadeIn   { "Fade In" };
      od::Parameter mFadeOut  { "Fade Out" };
      od::Parameter mResetTo  { "Reset To" };
      od::Parameter *mpProxyLength;

      od::Option mResetFlags { "Reset Flags", 0b00 };
#endif

      enum State {
        Waiting,
        Playing,
        Recording,
        Overdubbing
      };

      State state() {
        if (mRecordLatch.state())  return Recording;
        if (mOverdubLatch.state()) return Overdubbing;
        if (mEngageLatch.state())  return Playing;
        return Waiting;
      }

      float recordLevel() {
        return mFadeSlew.value();
      }

      int numberOfClockMarks() {
        auto length = mRecordLatch.state() ? mEngageLatch.count() : mpProxyLength->value();
        return min(length + 1, mClockMarks.size());
      }

      int getClockMark(int step) {
        if ((size_t)step < mClockMarks.size()) {
          return mClockMarks.at(step);
        }

        return 0;
      }

    private:
      inline void reset(int step, bool shadow) {
        int actualStep = mod(step, numberOfClockMarks() - 1);
        mEngageLatch.setCount(actualStep);
        mShadowIndex  = mCurrentIndex;
        mCurrentIndex = getClockMark(actualStep);
        if (shadow) mShadowSlew.setValue(1);
      }

      inline void markClock() {
        int step = mEngageLatch.count();
        mClockMarks.resize(max(step + 1, mClockMarks.size()));
        mClockMarks[step] = mCurrentIndex;

        if (!mpSlices) return;

        if (step >= mpSlices->size()) {
          mpSlices->append(od::Slice { mCurrentIndex });
        } else {
          od::Slice *pSlice = mpSlices->get(step);
          if (pSlice) pSlice->mStart = mCurrentIndex;
        }
      }

      inline float32x4_t read(const int* index, int channel) const {
        return readSample(mpSample, index, channel);
      }

      inline void write(const int* index, int channel, float32x4_t value) {
        writeSample(mpSample, index, channel, value);
      }

      inline int channels() const {
        return mStereo ? 2 : 1;
      }

      bool mStereo;
      od::Slices *mpSlices = 0;
      std::vector<int> mClockMarks;

      Slew mFadeSlew { 0, 1, 1 };
      Slew mShadowSlew { 0, 1, 1 };
      SyncLatch mClockLatch;
      SyncLatch mResetLatch;
      SyncLatch mRecordLatch;
      SyncLatch mOverdubLatch;
      SyncLatch mEngageLatch;

      inline void updateFades() {
        float sp          = globalConfig.samplePeriod;
        float fadeTime    = fmax(mFade.value(), sp);
        float fadeInTime  = fmax(mFadeIn.value(), sp);
        float fadeOutTime = fmax(mFadeOut.value(), sp);

        mFadeSlew.setRiseFall(sp / fadeInTime, sp / fadeOutTime);
        mShadowSlew.setRiseFall(1, sp / fadeTime);
      }

      struct Buffers {
        float *mpIn[2], *mpOut[2];
        float *mpClock, *mpEngage, *mpRecord, *mpOverdub, *mpReset, *mpLength, *mpRlength;

        inline Buffers(Sloop &self) {
          int channels = self.channels();
          for (int channel = 0; channel < channels; channel++) {
            mpIn[channel]  = self.getInput(channel)->buffer();
            mpOut[channel] = self.getOutput(channel)->buffer();
          }

          mpClock   = self.mClock.buffer();
          mpEngage  = self.mEngage.buffer();
          mpRecord  = self.mRecord.buffer();
          mpOverdub = self.mOverdub.buffer();
          mpReset   = self.mReset.buffer();
          mpLength  = self.mLength.buffer();
          mpRlength = self.mRLength.buffer();
        }

        inline float32x4_t in(int channel, int offset) const {
          return vld1q_f32(mpIn[channel]  + offset);
        }

        inline void out(int channel, int offset, float32x4_t value) {
          vst1q_f32(mpOut[0] + offset, value);
        }

        inline float32x4_t clock(int offset)   const { return vld1q_f32(mpClock   + offset); }
        inline float32x4_t engage(int offset)  const { return vld1q_f32(mpEngage  + offset); }
        inline float32x4_t record(int offset)  const { return vld1q_f32(mpRecord  + offset); }
        inline float32x4_t overdub(int offset) const { return vld1q_f32(mpOverdub + offset); }
        inline float32x4_t reset(int offset)   const { return vld1q_f32(mpReset   + offset); }
        inline float32x4_t length(int offset)  const { return vld1q_f32(mpLength  + offset); }
        inline float32x4_t rlength(int offset) const { return vld1q_f32(mpRlength + offset); }
      };

      struct Constants {
        float32x4_t zero = vdupq_n_f32(0);
        float32x4_t half = vdupq_n_f32(0.5); 
        float32x4_t one  = vdupq_n_f32(1);

        float32x4_t feedback, through;

        int resetTo;
        bool resetOnDisengage, resetOnOverdub;

        inline Constants(Sloop &self) {
          feedback         = vdupq_n_f32(self.mFeedback.value());
          through          = vdupq_n_f32(self.mThrough.value());
          resetTo          = (int)self.mResetTo.value();
          resetOnDisengage = self.mResetFlags.getFlag(0);
          resetOnOverdub   = self.mResetFlags.getFlag(1);
        }
      };

      struct LoadedVectors {
        int32_t  length[4], rlength[4];
        uint32_t clock[4], engage[4], record[4], overdub[4], reset[4];

        inline LoadedVectors(const Buffers& buffers, const Constants &constants, int offset) {
          vst1q_s32(length,  vcvtq_s32_f32(buffers.length(offset)));
          vst1q_s32(rlength, vcvtq_s32_f32(buffers.rlength(offset)));
          vst1q_u32(clock,       vcgtq_f32(buffers.clock(offset),   constants.zero));
          vst1q_u32(engage,      vcgtq_f32(buffers.engage(offset),  constants.zero));
          vst1q_u32(record,      vcgtq_f32(buffers.record(offset),  constants.zero));
          vst1q_u32(overdub,     vcgtq_f32(buffers.overdub(offset), constants.zero));
          vst1q_u32(reset,       vcgtq_f32(buffers.reset(offset),   constants.zero));
        }
      };

      inline void updateLatches(const LoadedVectors &v, int step) {
        bool sync = mClockLatch.read(v.clock[step]);
        mEngageLatch.readSync(v.engage[step], sync);
        mRecordLatch.readSync(v.record[step], sync);
        mOverdubLatch.readSyncMax(v.overdub[step], sync, v.rlength[step]);
        mResetLatch.readSync(v.reset[step], sync);
      }

      struct ProcessedStep {
        bool isRecord;
        int currentIndex, shadowIndex;
        float inputLevel, shadowLevel;
      };

      inline ProcessedStep processStep(const Constants &constants, int length) {
        bool engaged = mEngageLatch.state();
        bool record  = mRecordLatch.state();
        bool overdub = mOverdubLatch.state();
        bool doReset = false;

        if (!engaged) doReset = constants.resetOnDisengage;
        else if (mClockLatch.state()) {
          markClock();

          if (record) {
            length = mRecordLatch.count() + 1;
            mpProxyLength->hardSet(length);
          }

          doReset = mRecordLatch.firstOrLast()
                  || (constants.resetOnOverdub && mOverdubLatch.first())
                  || mEngageLatch.count() >= length
                  || mResetLatch.state();
        }

        if (doReset) reset(constants.resetTo, !(record || overdub));

        return ProcessedStep {
          record,
          mCurrentIndex = (mCurrentIndex + engaged) % mEndIndex,
          mShadowIndex  = (mShadowIndex  + engaged) % mEndIndex,
          mFadeSlew.move(record || overdub),
          mShadowSlew.move(0)
        };
      }

      struct ProcessedIndices {
        int current[4], shadow[4];

        inline ProcessedIndices(const ProcessedStep *steps) {
          for (int i = 0; i < 4; i++) {
            current[i] = steps[i].currentIndex;
            shadow[i]  = steps[i].shadowIndex;
          }
        }
      };

      struct ProcessedLevels {
        Complement input, shadow;
        float32x4_t feedback, through;

        inline ProcessedLevels(const ProcessedStep *steps, const Constants& constants) {
          float notRecord[4], inputLevel[4], shadowLevel[4];
          for (int i = 0; i < 4; i++) {
            notRecord[i]   = (int)!steps[i].isRecord;
            inputLevel[i]  = steps[i].inputLevel;
            shadowLevel[i] = steps[i].shadowLevel;
          }

          input    = Complement { vld1q_f32(inputLevel), constants.one };
          shadow   = Complement { vld1q_f32(shadowLevel), constants.one };
          feedback = vld1q_f32(notRecord) * input.lerpToOne(constants.feedback);
          through  = constants.through * input.mComplement;
        }
      };

      struct ProcessedInput {
        ProcessedIndices indices;
        ProcessedLevels levels;
      };

      inline ProcessedInput processInput(const Buffers& buffers, const Constants &constants, int offset) {
        LoadedVectors v { buffers, constants, offset };

        ProcessedStep steps[4];
        for (int j = 0; j < 4; j++) {
          updateLatches(v, j);
          steps[j] = processStep(constants, v.length[j]);
        }

        return ProcessedInput {
          ProcessedIndices { steps },
          ProcessedLevels { steps, constants }
        };
      }

      inline float32x4_t processSample(const int *index, int channel, const ProcessedLevels &levels, float32x4_t input) {
        float32x4_t original = read(index, channel);

        float32x4_t update;
        update = vmlaq_f32(input, original, levels.feedback);
        update = levels.input.lerp(update, original);
        update = levels.shadow.lerp(original, update);
        write(index, channel, update);

        return update;
      }

      inline float32x4_t processShadow(const int *index, int channel, const ProcessedLevels &levels, float32x4_t input) {
        float32x4_t original = read(index, channel);

        float32x4_t update;
        update = vmlaq_f32(input, original, levels.feedback);
        update = levels.input.lerp(update, original);
        update = levels.shadow.lerp(update, original);
        write(index, channel, update);

        return update;
      }

      inline void processMonoToMono(Buffers &buffers, const Constants &constants, const ProcessedInput &p, int offset) {
        float32x4_t input = buffers.in(0, offset);

        float32x4_t current = processSample(p.indices.current, 0, p.levels, input);
        float32x4_t shadow  = processShadow(p.indices.shadow,  0, p.levels, input);

        float32x4_t mix = p.levels.shadow.lerp(shadow, current);

        buffers.out(0, offset, vmlaq_f32(mix, input, p.levels.through));
      }

      inline void processMonoToStereo(Buffers &buffers, const Constants &constants, const ProcessedInput &p, int offset) {
        float32x4_t input = buffers.in(0, offset);

        float32x4_t lCurrent = processSample(p.indices.current, 0, p.levels, input);
        float32x4_t rCurrent = processSample(p.indices.current, 1, p.levels, input);
        float32x4_t lShadow  = processSample(p.indices.shadow,  0, p.levels, input);
        float32x4_t rShadow  = processSample(p.indices.shadow,  1, p.levels, input);

        float32x4_t left  = p.levels.shadow.lerp(lShadow, lCurrent);
        float32x4_t right = p.levels.shadow.lerp(rShadow, rCurrent);
        float32x4_t mix   = constants.half * (left + right);

        buffers.out(0, offset, vmlaq_f32(mix, input, p.levels.through));
      }

      inline void processStereoToMono(Buffers &buffers, const Constants &constants, const ProcessedInput &p, int offset) {
        float32x4_t lInput = buffers.in(0, offset);
        float32x4_t rInput = buffers.in(1, offset);
        float32x4_t input  = constants.half * (lInput + rInput);

        float32x4_t current = processSample(p.indices.current, 0, p.levels, input);
        float32x4_t shadow  = processSample(p.indices.shadow, 0, p.levels, input);

        float32x4_t mix = p.levels.shadow.lerp(shadow, current);

        buffers.out(0, offset, vmlaq_f32(mix, lInput, p.levels.through));
        buffers.out(1, offset, vmlaq_f32(mix, rInput, p.levels.through));
      }

      inline void processStereoToStereo(Buffers &buffers, const Constants &constants, const ProcessedInput &p, int offset) {
        float32x4_t lInput = buffers.in(0, offset);
        float32x4_t rInput = buffers.in(1, offset);

        float32x4_t lCurrent = processSample(p.indices.current, 0, p.levels, lInput);
        float32x4_t rCurrent = processSample(p.indices.current, 1, p.levels, rInput);
        float32x4_t lShadow  = processSample(p.indices.shadow,  0, p.levels, lInput);
        float32x4_t rShadow  = processSample(p.indices.shadow,  1, p.levels, rInput);

        float32x4_t lMix = p.levels.shadow.lerp(lShadow, lCurrent);
        float32x4_t rMix = p.levels.shadow.lerp(rShadow, rCurrent);

        buffers.out(0, offset, vmlaq_f32(lMix, lInput, p.levels.through));
        buffers.out(1, offset, vmlaq_f32(rMix, rInput, p.levels.through));
      }
  };
}