#pragma once

#include <od/objects/heads/TapeHead.h>
#include <od/extras/Random.h>
#include <od/audio/Slice.h>
#include <od/audio/Sample.h>
#include <od/audio/Slices.h>
#include <ClockMarks.h>
#include <SyncLatch.h>
#include <Slew.h>
#include <sstream>
#include <vector>
#include <util/math.h>

namespace sloop {

  #define RESET_MODE_JUMP 1
  #define RESET_MODE_STEP 2
  #define RESET_MODE_RANDOM 3

  class Sloop : public od::TapeHead {
    public:
      Sloop(od::Parameter *pLength, od::Parameter *pRecordLength, bool stereo) {
        mpProxyLength = pLength;
        mpProxyLength->attach();

        mpProxyDubLength = pRecordLength;
        mpProxyDubLength->attach();

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
        addInput(mDubLength);

        addOutput(mResetOut);

        addParameter(mThrough);
        addParameter(mFeedback);
        addParameter(mFade);
        addParameter(mFadeIn);
        addParameter(mFadeOut);
        addParameter(mResetTo);

        addOption(mLockDubLength);
        addOption(mResetFlags);
        addOption(mSliceMode);
        addOption(mResetMode);
      }

      virtual ~Sloop() {
        mpProxyLength->release();
        mpProxyDubLength->release();
        if (mpSlices) mpSlices->release();
      }

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

#ifndef SWIGLUA
      virtual void process();

      od::Inlet mClock     { "Clock" };
      od::Inlet mEngage    { "Engage" };
      od::Inlet mRecord    { "Record" };
      od::Inlet mOverdub   { "Overdub" };
      od::Inlet mReset     { "Reset" };
      od::Inlet mLength    { "Length" };
      od::Inlet mDubLength { "Overdub Length" };

      od::Outlet mResetOut { "Reset Out" };

      od::Parameter mThrough  { "Through",  1.0 };
      od::Parameter mFeedback { "Feedback", 1.0 };
      od::Parameter mFade     { "Fade",     0.005 };
      od::Parameter mFadeIn   { "Fade In",  0.01 };
      od::Parameter mFadeOut  { "Fade Out", 0.1 };
      od::Parameter mResetTo  { "Reset To", 0 };

      od::Parameter *mpProxyLength = 0;
      od::Parameter *mpProxyDubLength = 0;

      od::Option mLockDubLength { "Lock Overdub Length", CHOICE_YES };
      od::Option mResetFlags { "Reset Flags", 0b010 };
      od::Option mSliceMode { "Slice Mode", 0b00 };
      od::Option mResetMode { "Reset Mode", RESET_MODE_JUMP };
#endif

      ClockMarks& getClockMarks() {
        return marks;
      }

      void setCurrentStep(int step) {
        mEngageLatch.setCount(step);
        int position = step + 1 > marks.size() ? 0 : marks.get(step);
        mCurrentIndex = mShadowIndex = position;
      }

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

      inline float writeLevel() {
        return fmax(mRecordSlew.value(), mOverdubSlew.value());
      }

      inline int currentLength() {
        if (mRecordLatch.state()) return currentStep();
        return mpProxyLength->value();
      }

      inline int visibleMarks() {
        return util::min(currentLength() + 1, marks.size());
      }

      inline int markAt(int step) {
        return marks.get(step);
      }

      inline int wrappedStep(int step) {
        int v = visibleMarks() - 1;
        return v <= 0 ? 0 : util::mod(step, v);
      }

      inline int clampedStep(int step) {
        if (step < 0) step = 0;
        if (step > marks.size()) step = marks.size() - 1;
        return step;
      }

      inline bool willManualReset() {
        return mResetLatch.triggered() && !willRecord();
      }

      inline bool willRecord() {
        return mRecordLatch.triggered() && mRecordLatch.first();
      }

      inline int resetStep() {
        return willManualReset() ? mManualResetStep : 0;
      }

      inline int resetPosition() {
        return marks.get(resetStep());
      }

      inline int lastPosition() {
        return marks.get(clampedStep(currentLength()));
      }

      inline int currentStep() {
        return mEngageLatch.count();
      }

      inline bool isResetPending() {
        bool willOverdub  = mOverdubLatch.triggered() && mOverdubLatch.first();
        bool isEndOfCycle = currentStep() >= currentLength() - 1;

        return willRecord()
            || (resetOnOverdub() && willOverdub)
            || (resetOnEndOfCycle() && isEndOfCycle)
            || willManualReset();
      }
      
      inline void zeroBuffer() {
        if (mpSample) mpSample->zero();
      }

      inline int clearSlices() {
        int count = 0;

        if (mpSlices) {
          count = mpSlices->size();
          mpSlices->clear();
        }

        return count;
      }

      inline int createSlices() {
        int visible = visibleMarks();

        int count = 0;
        for (int step = 0; step < visible; step++) {
          count += insertSlice(step);
        }

        return count;
      }

    private:
      bool mStereo = false;
      ClockMarks marks;
      od::Slices *mpSlices = 0;
      int mManualResetStep = 0;

      SyncLatch mClockLatch;
      SyncLatch mResetLatch;
      SyncLatch mRecordLatch;
      SyncLatch mOverdubLatch;
      SyncLatch mEngageLatch;

      Slew mRecordSlew;
      Slew mOverdubSlew;
      Slew mShadowSlew;

      inline bool resetOnDisengage()  { return mResetFlags.getFlag(0); }
      inline bool resetOnEndOfCycle() { return mResetFlags.getFlag(1); }
      inline bool resetOnOverdub()    { return mResetFlags.getFlag(2); }
      inline bool sliceModeClock()    { return mSliceMode.getFlag(0); }
      inline bool sliceModeReset()    { return mSliceMode.getFlag(1); }

      inline void reset(bool manual, bool shadow) {
        int step = manual ? mManualResetStep : 0;

        mEngageLatch.setCount(step);
        mRecordLatch.setCount(0);
        mShadowIndex  = mCurrentIndex;
        mCurrentIndex = marks.get(step);

        if (shadow) mShadowSlew.setValue(1);
        if (sliceModeReset()) createSlices();

        updateManualResetStep(true);
      }

      inline void markClock() {
        int step = currentStep();
        marks.set(step, mCurrentIndex);
        if (sliceModeClock()) insertSlice(step);
      }

      inline bool insertSlice(int step) {
        if (!mpSlices) return false;

        int mark = marks.get(step);

        if (step >= mpSlices->size()) {
          mpSlices->append(od::Slice { mark});
        } else {
          od::Slice *pSlice = mpSlices->get(step);
          if (pSlice) pSlice->mStart = mark;
        }

        return true;
      }

      inline float32x4_t read(const int* index, int channel) const {
        return util::simd::readSample(mpSample, index, channel);
      }

      inline void write(const int* index, int channel, float32x4_t value) {
        util::simd::writeSample(mpSample, index, channel, value);
      }

      inline int channels() const {
        return mStereo ? 2 : 1;
      }

      inline void updateFades() {
        float sp      = globalConfig.samplePeriod;
        float fade    = sp / fmax(mFade.value(), sp);
        float fadeIn  = sp / fmax(mFadeIn.value(), sp);
        float fadeOut = sp / fmax(mFadeOut.value(), sp);

        mRecordSlew.setRiseFall(fadeIn, fadeOut);
        mOverdubSlew.setRiseFall(fadeIn, fadeOut);
        mShadowSlew.setRiseFall(1, fade);
      }

      inline void updateDubLengthLock() {
        bool shouldBeLocked = mLockDubLength.value() == CHOICE_YES;
        bool isLocked       = mpProxyDubLength->isTied();

        if (!isLocked && shouldBeLocked) mpProxyDubLength->tie(*mpProxyLength);
        if (isLocked && !shouldBeLocked) mpProxyDubLength->untie();
      }

      inline void updateManualResetStep(bool force) {
        int  current = currentStep();
        int  resetTo = mResetTo.value();
        int  mode    = mResetMode.value();
        int  step    = mManualResetStep;

        switch (mode) {
          case RESET_MODE_JUMP:
            step = resetTo;
            break;

          case RESET_MODE_STEP:
            step = current + resetTo;
            break;

          case RESET_MODE_RANDOM:
            float rand = od::Random::generateFloat(-1.0f, 1.0f);
            if (force) step = current + rand * resetTo;
            break;
        }

        mManualResetStep = wrappedStep(step);
      }

      struct Buffers {
        float *mpIn[2], *mpOut[2], *mpResetOut;
        float *mpClock, *mpEngage, *mpRecord, *mpOverdub, *mpReset, *mpLength, *mpDubLength;

        inline Buffers(Sloop &self) {
          int channels = self.channels();
          for (int channel = 0; channel < channels; channel++) {
            mpIn[channel]  = self.getInput(channel)->buffer();
            mpOut[channel] = self.getOutput(channel)->buffer();
          }

          mpClock     = self.mClock.buffer();
          mpEngage    = self.mEngage.buffer();
          mpRecord    = self.mRecord.buffer();
          mpOverdub   = self.mOverdub.buffer();
          mpReset     = self.mReset.buffer();
          mpLength    = self.mLength.buffer();
          mpDubLength = self.mDubLength.buffer();
          mpResetOut  = self.mResetOut.buffer();
        }

        inline float32x4_t in(int channel, int offset) const {
          return vld1q_f32(mpIn[channel]  + offset);
        }

        inline void out(int channel, int offset, float32x4_t value) {
          vst1q_f32(mpOut[channel] + offset, value);
        }

        inline float32x4_t clock(int offset)  const    { return vld1q_f32(mpClock     + offset); }
        inline float32x4_t engage(int offset) const    { return vld1q_f32(mpEngage    + offset); }
        inline float32x4_t record(int offset) const    { return vld1q_f32(mpRecord    + offset); }
        inline float32x4_t overdub(int offset) const   { return vld1q_f32(mpOverdub   + offset); }
        inline float32x4_t reset(int offset) const     { return vld1q_f32(mpReset     + offset); }
        inline float32x4_t length(int offset) const    { return vld1q_f32(mpLength    + offset); }
        inline float32x4_t dubLength(int offset) const { return vld1q_f32(mpDubLength + offset); }

        inline void resetOut(int offset, float32x4_t value) {
          vst1q_f32(mpResetOut + offset, value);
        }
      };

      struct Constants {
        float32x4_t zero = vdupq_n_f32(0);
        float32x4_t half = vdupq_n_f32(0.5); 
        float32x4_t one  = vdupq_n_f32(1);

        float32x4_t feedback, through;

        int resetTo, resetMode;
        bool resetOnDisengage, resetOnEndOfCycle, resetOnOverdub;

        inline Constants(Sloop &self) {
          feedback          = vdupq_n_f32(self.mFeedback.value());
          through           = vdupq_n_f32(self.mThrough.value());
          resetTo           = (int)self.mResetTo.value();
          resetMode         = self.mResetMode.value();
          resetOnDisengage  = self.resetOnDisengage();
          resetOnEndOfCycle = self.resetOnEndOfCycle();
          resetOnOverdub    = self.resetOnOverdub();
        }
      };

      struct LoadedVectors {
        int32_t  length[4], dubLength[4];
        uint32_t clock[4], engage[4], record[4], overdub[4], reset[4];

        inline LoadedVectors(const Buffers& buffers, const Constants &constants, int offset) {
          vst1q_s32(length,    vcvtq_s32_f32(buffers.length(offset)));
          vst1q_s32(dubLength, vcvtq_s32_f32(buffers.dubLength(offset)));
          vst1q_u32(clock,         vcgtq_f32(buffers.clock(offset),   constants.zero));
          vst1q_u32(engage,        vcgtq_f32(buffers.engage(offset),  constants.zero));
          vst1q_u32(record,        vcgtq_f32(buffers.record(offset),  constants.zero));
          vst1q_u32(overdub,       vcgtq_f32(buffers.overdub(offset), constants.zero));
          vst1q_u32(reset,         vcgtq_f32(buffers.reset(offset),   constants.zero));
        }
      };

      inline void updateLatches(const LoadedVectors &v, int step) {
        bool sync = mClockLatch.readTrigger(v.clock[step]);
        mEngageLatch.readGateSyncCount(v.engage[step] && !mPaused, sync);
        mRecordLatch.readSyncCount(v.record[step], sync);
        mOverdubLatch.readSyncCountMax(v.overdub[step], sync, v.dubLength[step]);
        mResetLatch.readTriggerSync(v.reset[step] || mResetRequested, sync);
        mResetRequested = false;
      }

      struct ProcessedStep {
        bool isReset;
        int currentIndex, shadowIndex;
        float recordLevel, overdubLevel, shadowLevel;
      };

      inline ProcessedStep processStep(const Constants &constants, int length) {
        bool engaged     = mEngageLatch.state();
        bool record      = mRecordLatch.state();
        bool overdub     = mOverdubLatch.state();
        bool manualReset = mResetLatch.state();
        bool doReset     = false;

        bool isManualReset = false;
        bool isShadowReset = true;

        if (!engaged) {
          doReset = constants.resetOnDisengage;
        } else if (mClockLatch.state()) {
          markClock();

          if (record) {
            length = mRecordLatch.count() + 1;
            mpProxyLength->hardSet(length);
          }

          bool onRecord  = mRecordLatch.firstOrLast();
          bool onOverdub = constants.resetOnOverdub && mOverdubLatch.first();
          bool onEoc     = constants.resetOnEndOfCycle && mEngageLatch.count() >= length;
          bool onManual  = manualReset && !record;
          doReset = onRecord || onOverdub || onEoc || onManual;

          isManualReset = onManual;
          isShadowReset = !record;
        }

        if (doReset) reset(isManualReset, isShadowReset);

        mCurrentIndex += engaged;
        mShadowIndex  += engaged;
        if (mCurrentIndex >= mEndIndex) mCurrentIndex = 0;
        if (mShadowIndex  >= mEndIndex) mShadowIndex = 0;

        float recordLevel  = mRecordSlew.move(record);
        float overdubLevel = mOverdubSlew.move(overdub);
        float shadowLevel  = mShadowSlew.move(0);

        if (recordLevel + overdubLevel > 0) mpSample->mDirty = true;

        return ProcessedStep {
          doReset,
          mCurrentIndex,
          mShadowIndex,
          recordLevel,
          overdubLevel,
          shadowLevel
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
        util::simd::Complement input, shadow;
        float32x4_t resetOut, feedback, through;

        inline ProcessedLevels(const ProcessedStep *steps, const Constants& constants) {
          float reset[4], recordLevel[4], overdubLevel[4], shadowLevel[4];
          for (int i = 0; i < 4; i++) {
            reset[i]        = steps[i].isReset ? 1.0f : 0.0f;
            recordLevel[i]  = steps[i].recordLevel;
            overdubLevel[i] = steps[i].overdubLevel;
            shadowLevel[i]  = steps[i].shadowLevel;
          }

          util::simd::Complement record  { vld1q_f32(recordLevel), constants.one };
          util::simd::Complement overdub { vld1q_f32(overdubLevel), constants.one };
          float32x4_t max = vmaxq_f32(record.mValue, overdub.mValue);

          input    = util::simd::Complement { max, constants.one };
          shadow   = util::simd::Complement { vld1q_f32(shadowLevel), constants.one };
          feedback = record.mComplement * input.ease(constants.feedback);
          through  = constants.through * input.mComplement;
          resetOut = vld1q_f32(reset);
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

      inline void processResetOut(Buffers &buffers, const ProcessedInput &p, int offset) {
        buffers.resetOut(offset, p.levels.resetOut);
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
        float32x4_t lShadow  = processShadow(p.indices.shadow,  0, p.levels, input);
        float32x4_t rShadow  = processShadow(p.indices.shadow,  1, p.levels, input);

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
        float32x4_t shadow  = processShadow(p.indices.shadow, 0, p.levels, input);

        float32x4_t mix = p.levels.shadow.lerp(shadow, current);

        buffers.out(0, offset, vmlaq_f32(mix, lInput, p.levels.through));
        buffers.out(1, offset, vmlaq_f32(mix, rInput, p.levels.through));
      }

      inline void processStereoToStereo(Buffers &buffers, const Constants &constants, const ProcessedInput &p, int offset) {
        float32x4_t lInput = buffers.in(0, offset);
        float32x4_t rInput = buffers.in(1, offset);

        float32x4_t lCurrent = processSample(p.indices.current, 0, p.levels, lInput);
        float32x4_t rCurrent = processSample(p.indices.current, 1, p.levels, rInput);
        float32x4_t lShadow  = processShadow(p.indices.shadow,  0, p.levels, lInput);
        float32x4_t rShadow  = processShadow(p.indices.shadow,  1, p.levels, rInput);

        float32x4_t lMix = p.levels.shadow.lerp(lShadow, lCurrent);
        float32x4_t rMix = p.levels.shadow.lerp(rShadow, rCurrent);

        buffers.out(0, offset, vmlaq_f32(lMix, lInput, p.levels.through));
        buffers.out(1, offset, vmlaq_f32(rMix, rInput, p.levels.through));
      }
  };
}