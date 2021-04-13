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

        mpSlices = slices;
        // od::Slices *pSlices = mpSlices;
        // mpSlices = 0;
        // if (pSlices) pSlices->release();

        // pSlices = slices;
        // //if (pSlices) pSlices->attach();
        // mpSlices = pSlices;
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

      bool willReset() {
        return false;
      }

      float recordLevel() {
        return mFadeSlew.value();
      }

      int numberOfClockMarks() {
        auto length = mRecordLatch.state() ? mCurrentStep : mpProxyLength->value();
        return min(length + 1, mClockMarks.size());
      }

      int getClockMark(int step) {
        if ((size_t)step < mClockMarks.size()) {
          return mClockMarks.at(step);
        }

        return 0;
      }

    private:
      void reset(int step, bool shadow) {
        int actualStep = mod(step, numberOfClockMarks() - 1);
        mCurrentStep  = actualStep;
        mShadowIndex  = mCurrentIndex;
        mCurrentIndex = getClockMark(actualStep);
        if (shadow) mShadowSlew.setValue(1);
      }

      void markClock() {
        mClockMarks.resize(max(mCurrentStep + 1, mClockMarks.size()));
        mClockMarks[mCurrentStep] = mCurrentIndex;

        logDebug(1, "[%d, %d]", mCurrentStep, mCurrentIndex);

        int size = mpSlices->size();
        if (mCurrentStep >= size) {
          mpSlices->append(od::Slice { mCurrentIndex });
        } else {
          od::Slice *pSlice = mpSlices->get(mCurrentStep);
          if (pSlice) pSlice->mStart = mCurrentIndex;
        }
      }

      int channels() { return mStereo ? 2 : 1; }
      bool mStereo;
      Slew mFadeSlew { 0, 1, 1 };
      Slew mShadowSlew { 0, 1, 1 };
      SyncLatch mClockLatch;
      SyncLatch mResetLatch;
      SyncLatch mRecordLatch;
      SyncLatch mOverdubLatch;
      SyncLatch mEngageLatch;

      od::Slices *mpSlices;

      // zero is the last step
      int mCurrentStep  = -1;
      int mRecordCount  = 0;
      int mOverdubCount = 0;

      std::vector<int> mClockMarks;

      inline float32x4_t getSampleVector(const int *index, int channel) {
        float fValue[4];

        for (int i = 0; i < 4; i++) {
          fValue[i] = mpSample->get(index[i], channel);
        }

        return vld1q_f32(fValue);
      }

      inline void setSampleVector(const int *index, int channel, float32x4_t value) {
        float fValue[4];
        vst1q_f32(fValue, value);

        for (int i = 0; i < 4; i++) {
          //logDebug(1,"[%d, %d] = %f", index[i], channel, fValue[i]);
          mpSample->set(index[i], channel, fValue[i]);
        }
      }

      struct FrameConst {
        float *clock;
        float *engage;
        float *record;
        float *overdub;
        float *reset;
        float *length;
        float *rlength;

        bool resetOnDisengage;
        bool resetOnOverdub;

        float32x4_t zero;
        float32x4_t one;
        float32x4_t feedback;
        float32x4_t through;
        int resetTo;
      };

      struct ProcessedLatches {
        bool clock;
        bool engaged;
        bool record;
        bool overdub;
        bool reset;

        inline bool isWrite() const {
          return record || overdub;
        }
      };

      struct ProcessLatches {
        uint32_t clock[4];
        uint32_t engage[4];
        uint32_t record[4];
        uint32_t overdub[4];
        uint32_t reset[4];

        inline ProcessLatches(const FrameConst &frame, int offset) {
          vst1q_u32(clock,   vcgtq_f32(vld1q_f32(frame.clock   + offset), frame.zero));
          vst1q_u32(engage,  vcgtq_f32(vld1q_f32(frame.engage  + offset), frame.zero));
          vst1q_u32(record,  vcgtq_f32(vld1q_f32(frame.record  + offset), frame.zero));
          vst1q_u32(overdub, vcgtq_f32(vld1q_f32(frame.overdub + offset), frame.zero));
          vst1q_u32(reset,   vcgtq_f32(vld1q_f32(frame.reset   + offset), frame.zero));
        }

        inline ProcessedLatches process(Sloop &self, int j) {
          bool sync = self.mClockLatch.read(clock[j]);
          return ProcessedLatches {
            sync,
            self.mEngageLatch.readSync(engage[j], sync),
            self.mRecordLatch.readSync(record[j], sync),
            self.mOverdubLatch.readSync(overdub[j], sync),
            self.mResetLatch.readSync(reset[j], sync)
          };
        }
      };

      struct ProcessedLengths {
        int normal;
        int record;
      };

      struct ProcessLengths {
        int32_t normal[4], record[4];

        inline ProcessLengths(const FrameConst &frame, int offset) {
          vst1q_s32(normal,  vcvtq_s32_f32(vld1q_f32(frame.length  + offset)));
          vst1q_s32(record, vcvtq_s32_f32(vld1q_f32(frame.rlength + offset)));
        }

        inline ProcessedLengths process(int j) {
          return ProcessedLengths {
            normal[j],
            record[j]
          };
        }
      };

      struct ProcessedClock {
        int currentIndex;
        int shadowIndex;
        bool isRecord;
        float inputLevel;
        float shadowLevel;
      };

      struct ProcessClock {
        inline ProcessedClock process(
          Sloop &self,
          const FrameConst &frame,
          const ProcessedLatches &latch,
          const ProcessedLengths &length
        ) {
          bool doReset = false;

          if (latch.engaged) {
            if (latch.clock) {
              bool isRecordStart  = self.mRecordCount == 0;
              bool resetOnRecord  = latch.record ? isRecordStart : !isRecordStart;
              bool isOverdubStart = self.mOverdubCount == 0;
              bool resetOnOverdub = frame.resetOnOverdub && isOverdubStart;

              self.mRecordCount  = latch.record  ? self.mRecordCount  + 1 : 0;
              self.mOverdubCount = latch.overdub ? self.mOverdubCount + 1 : 0;
              self.mCurrentStep  = self.mCurrentStep + 1;

              int max         = latch.record ? self.mRecordCount : length.normal;
              bool isLastStep = self.mCurrentStep >= max;
              if (latch.record) self.mpProxyLength->hardSet(max);

              self.markClock();

              doReset = resetOnRecord
                     || resetOnOverdub
                     || isLastStep
                     || latch.reset;
            }
          } else {
            doReset = frame.resetOnDisengage;
          }

          self.mCurrentIndex = self.mCurrentIndex % self.mEndIndex;
          self.mShadowIndex  = self.mShadowIndex  % self.mEndIndex;

          if (doReset) self.reset(frame.resetTo, !latch.isWrite());

          return ProcessedClock {
            self.mCurrentIndex = self.mCurrentIndex + latch.engaged,
            self.mShadowIndex  = self.mShadowIndex  + latch.engaged,
            latch.record,
            self.mFadeSlew.move(latch.isWrite()),
            self.mShadowSlew.move(0)
          };
        }
      };

      struct ProcessedInput {
        int currentIndex[4];
        int shadowIndex[4];
        Complement inputLevel;
        Complement shadowLevel;
        float32x4_t feedbackLevel;
        float32x4_t throughLevel;

        ProcessedInput() {}
      };

      struct ProcessInput {
        inline ProcessedInput process(Sloop& self, const FrameConst &frame, int offset) {
          ProcessLatches latch { frame, offset };
          ProcessLengths length { frame, offset };
          ProcessClock   clock;

          ProcessedInput out;
          float notRecord[4], inputLevel[4], shadowLevel[4];

          for (int j = 0; j < 4; j++) {
            auto latchStep  = latch.process(self, j);
            auto lengthStep = length.process(j);
            auto clockStep  = clock.process(self, frame, latchStep, lengthStep);

            out.currentIndex[j] = clockStep.currentIndex;
            out.shadowIndex[j]  = clockStep.shadowIndex;
            notRecord[j]        = (int)!clockStep.isRecord;
            inputLevel[j]       = clockStep.inputLevel;
            shadowLevel[j]      = clockStep.shadowLevel;
          }

          out.inputLevel    = Complement { vld1q_f32(inputLevel),  frame.one };
          out.shadowLevel   = Complement { vld1q_f32(shadowLevel), frame.one };
          out.feedbackLevel = vld1q_f32(notRecord) * out.inputLevel.lerpToOne(frame.feedback);
          out.throughLevel  = frame.through * out.inputLevel.mComplement;

          return out;
        }
      };

      struct ProcessOutput {
        inline void process(Sloop &self, const FrameConst &frame, const ProcessedInput &p, int offset) {
          float *in  = self.getInput(0)->buffer();
          float *out = self.getOutput(0)->buffer();

          float32x4_t input = vld1q_f32(in + offset);

          float32x4_t current    = self.getSampleVector(p.currentIndex, 0);
          float32x4_t newCurrent = write(p, current, input);
          self.setSampleVector(p.currentIndex, 0, newCurrent);

          float32x4_t shadow    = self.getSampleVector(p.shadowIndex, 0);
          float32x4_t newShadow = write(p, shadow, input);
          self.setSampleVector(p.shadowIndex, 0, newShadow);

          float32x4_t output = p.shadowLevel.lerp(newShadow, newCurrent);
          vst1q_f32(out + offset, vmlaq_f32(output, p.throughLevel, input));
        }

        inline float32x4_t write(const ProcessedInput &p, float32x4_t original, float32x4_t input) {
          float32x4_t x;
          x = vmlaq_f32(input, original, p.feedbackLevel);
          x = p.inputLevel.lerp(x, original);
          x = p.shadowLevel.lerp(original, x);
          return x;
        }
      };
  };
}