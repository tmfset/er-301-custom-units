#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

#include <Register.h>
#include <OneTime.h>
#include <od/extras/Random.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/ops.h>
#include <hal/simd.h>

namespace lojik {

  Register::Register(int max, float randomize) {
    addInput(mIn);
    addOutput(mOut);

    addInput(mLength);
    addInput(mStride);

    addInput(mClock);
    addInput(mCapture);
    addInput(mShift);
    addInput(mReset);

    addParameter(mScatter);
    addParameter(mDrift);
    addParameter(mInputGain);
    addParameter(mInputBias);

    addOption(mMode);
    addOption(mSync);

    mState.setMax(max);
    mState.setScatter(randomize);
    mState.markRandomizeAll();
  }

  Register::~Register() { }

  void Register::process() {
    processTriggers();

    bool isSeqMode = MODE_SEQ == mMode.value();
    if (isSeqMode) processSeq();
    else           processNormal();
  }

  void Register::processNormal() {
    float *in         = mIn.buffer();
    float *out        = mOut.buffer();
    float *length     = mLength.buffer();
    float *stride     = mStride.buffer();
    float *clock      = mClock.buffer();
    float *capture    = mCapture.buffer();
    float *shift      = mShift.buffer();
    float *reset      = mReset.buffer();

    OneTime clockSwitch   { mClockSwitch,   false };
    OneTime shiftSwitch   { mShiftSwitch,   mSync.getFlag(0) };
    OneTime captureSwitch { mCaptureSwitch, mSync.getFlag(1) };
    OneTime resetSwitch   { mResetSwitch,   mSync.getFlag(2) };

    mState.setScatter(mScatter.value());
    mState.setDrift(mDrift.value());
    mState.setGain(mInputGain.value());
    mState.setBias(mInputBias.value());

    float32x4_t zero = vdupq_n_f32(0);

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      // According to some video I watched, loading all in one step
      // is slightly faster. I didn't test it though and who knows
      // what the optimizer will do.
      float32x4_t loadLength  = vld1q_f32(length  + i);
      float32x4_t loadStride  = vld1q_f32(stride  + i);
      float32x4_t loadClock   = vld1q_f32(clock   + i);
      float32x4_t loadShift   = vld1q_f32(shift   + i);
      float32x4_t loadCapture = vld1q_f32(capture + i);
      float32x4_t loadReset   = vld1q_f32(reset   + i);

      int32_t iLength[4], iStride[4];
      uint32_t isClockHigh[4], isCaptureHigh[4], isShiftHigh[4], isResetHigh[4];

      vst1q_s32(iLength,   vcvtq_s32_f32(loadLength));
      vst1q_s32(iStride,   vcvtq_s32_f32(loadStride));
      vst1q_u32(isClockHigh,   vcgtq_f32(loadClock,   zero));
      vst1q_u32(isShiftHigh,   vcgtq_f32(loadShift,   zero));
      vst1q_u32(isCaptureHigh, vcgtq_f32(loadCapture, zero));
      vst1q_u32(isResetHigh,   vcgtq_f32(loadReset,   zero));

      for (int j = 0; j < 4; j++) {
        clockSwitch.mark(isClockHigh[j]);
        bool clock = clockSwitch.read();

        shiftSwitch.mark(isShiftHigh[j], clock);
        captureSwitch.mark(isCaptureHigh[j], clock);
        resetSwitch.mark(isResetHigh[j], clock);

        bool doShift   = shiftSwitch.read(clock);
        bool doCapture = captureSwitch.read(clock);
        bool doReset   = resetSwitch.read(clock);

        bool doStep    = clock && !doShift;
        bool doDrift   = doStep || doShift || doReset;

        mState.setLimit(iLength[j]);
        mState.setStride(iStride[j]);

        if (doStep)  mState.doStep();
        if (doShift) mState.doShift();
        if (doReset) mState.doReset();
        if (doDrift) mState.doDrift();

        uint32_t index = mState.current();
        if (doCapture) mState.setData(index, in[i + j]);

        out[i + j] = mState.data(index);
      }
    }

    mClockSwitch   = clockSwitch;
    mShiftSwitch   = shiftSwitch;
    mCaptureSwitch = captureSwitch;
    mResetSwitch   = resetSwitch;
  }

  void Register::processSeq() {
    float *in         = mIn.buffer();
    float *out        = mOut.buffer();
    float *stride     = mStride.buffer();
    float *clock      = mClock.buffer();
    float *capture    = mCapture.buffer();
    float *shift      = mShift.buffer();
    float *reset      = mReset.buffer();

    OneTime clockSwitch   { mClockSwitch,   false };
    OneTime shiftSwitch   { mShiftSwitch,   mSync.getFlag(0) };
    OneTime captureSwitch { mCaptureSwitch, false };
    OneTime resetSwitch   { mResetSwitch,   mSync.getFlag(2) };

    mState.setScatter(mScatter.value());
    mState.setDrift(mRecordSequence ? 0 : mDrift.value());
    mState.setGain(mInputGain.value());
    mState.setBias(mInputBias.value());

    float32x4_t zero = vdupq_n_f32(0);

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      // According to some video I watched, loading all in one step
      // is slightly faster. I didn't test it though and who knows
      // what the optimizer will do.
      float32x4_t loadStride  = vld1q_f32(stride  + i);
      float32x4_t loadClock   = vld1q_f32(clock   + i);
      float32x4_t loadShift   = vld1q_f32(shift   + i);
      float32x4_t loadCapture = vld1q_f32(capture + i);
      float32x4_t loadReset   = vld1q_f32(reset   + i);

      int32_t iStride[4];
      uint32_t isClockHigh[4], isCaptureHigh[4], isShiftHigh[4], isResetHigh[4];

      vst1q_s32(iStride,   vcvtq_s32_f32(loadStride));
      vst1q_u32(isClockHigh,   vcgtq_f32(loadClock,   zero));
      vst1q_u32(isShiftHigh,   vcgtq_f32(loadShift,   zero));
      vst1q_u32(isCaptureHigh, vcgtq_f32(loadCapture, zero));
      vst1q_u32(isResetHigh,   vcgtq_f32(loadReset,   zero));

      for (int j = 0; j < 4; j++) {
        clockSwitch.mark(isClockHigh[j]);
        // Don't listen to the clock while recording.
        bool clock = clockSwitch.read() && !mRecordSequence;

        shiftSwitch.mark(isShiftHigh[j], clock);
        captureSwitch.mark(isCaptureHigh[j], clock);
        resetSwitch.mark(isResetHigh[j], clock);

        bool isShift   = shiftSwitch.read(clock);
        bool isCapture = captureSwitch.read(clock);
        bool isReset   = resetSwitch.read(clock || mRecordSequence);

        bool doStep, doShift, doCapture, doReset, doDrift;

        if (mRecordSequence) {
          if (isCapture) mSequenceLength += 1;
          if (isReset)   mRecordSequence  = false;

          doStep    = isCapture;
          doShift   = false;
          doCapture = isCaptureHigh[j];
          doReset   = isReset;
          doDrift   = true;

          mState.setLimit(mState.max());
          mState.setStride(1);
        } else {
          if (isCapture) {
            mSequenceLength = 1;
            mRecordSequence = true;
          }

          doStep    = clock && !isShift;
          doShift   = isShift;
          doCapture = isCaptureHigh[j];
          doReset   = isReset || isCapture;
          doDrift   = doStep || isShift || isReset;

          mState.setLimit(mSequenceLength);
          mState.setStride(iStride[j]);
        }

        if (doStep)  mState.doStep();
        if (doShift) mState.doShift();
        if (doReset) mState.doReset();
        if (doDrift) mState.doDrift();

        uint32_t index = mState.current();
        if (doCapture) mState.setData(index, in[i + j]);

        out[i + j] = mState.data(index);
      }
    }

    mClockSwitch   = clockSwitch;
    mShiftSwitch   = shiftSwitch;
    mCaptureSwitch = captureSwitch;
    mResetSwitch   = resetSwitch;
  }

}