#include <Euclid.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/ops.h>
#include <hal/simd.h>
#include <sense.h>

namespace lojik {
  Euclid::Euclid(int max) {
    addInput(mClock);
    addInput(mReset);
    addInput(mRotate);
    addOutput(mOut);

    addParameter(mBeats);
    addParameter(mLength);
    addParameter(mShift);

    addOption(mSync);
    addOption(mMode);
    addOption(mSense);
    mMax = max;
  }

  Euclid::~Euclid() { }

  void Euclid::process() {
    setRythm(mBeats.value(), mLength.value());

    float *clock  = mClock.buffer();
    float *reset  = mReset.buffer();
    float *rotate = mRotate.buffer();
    float *out    = mOut.buffer();

    OneTime clockSwitch  { mClockSwitch,  false };
    OneTime rotateSwitch { mRotateSwitch, mSync.getFlag(0) };
    OneTime resetSwitch  { mResetSwitch,  mSync.getFlag(1) };
    OneTime trigSwitch   { mTrigSwitch,   false };

    int step  = mStep;
    int shift = mShift.value();
    int mode  = mMode.value();

    int startShift = shift;

    float32x4_t sense = vdupq_n_f32(getSense(mSense));
    float32x4_t zero = vdupq_n_f32(0.0f);

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      float32x4_t loadClock  = vld1q_f32(clock + i);
      float32x4_t loadReset  = vld1q_f32(reset + i);
      float32x4_t loadRotate = vld1q_f32(rotate + i);

      uint32_t isClockHigh[4], isResetHigh[4], isRotateHigh[4];
      vst1q_u32(isClockHigh,  vcgtq_f32(loadClock,  sense));
      vst1q_u32(isResetHigh,  vcgtq_f32(loadReset,  zero));
      vst1q_u32(isRotateHigh, vcgtq_f32(loadRotate, zero));

      for (int j = 0; j < 4; j++) {
        clockSwitch.mark(isClockHigh[j]);
        bool c = clockSwitch.read();

        rotateSwitch.mark(isRotateHigh[j], c);
        resetSwitch.mark(isResetHigh[j], c);

        bool doRotate = rotateSwitch.read(c);
        bool doReset  = resetSwitch.read(c);
        bool doStep   = c && !doRotate;

        if (doStep)   step  = index(step, 1);
        if (doRotate) shift = index(shift, 1);
        if (doReset)  step  = 0;

        bool moved = doRotate || doReset || doStep;
        bool isHigh = mRythm[index(step, -shift)];
        float value = 0.0f;
        if (isHigh) {
          switch (mode) {
            case EUCLID_MODE_TRIGGER:
              trigSwitch.mark(isHigh, moved);
              value = trigSwitch.read() ? 1.0f : 0.0f;
              break;

            case EUCLID_MODE_GATE:
              value = 1.0f;
              break;

            case EUCLID_MODE_PASSTHROUGH:
              value = loadClock[j];
              break;
          }
        }

        out[i + j] = value;
      }
    }

    mStep = step;
    int deltaShift = shift - startShift;
    if (deltaShift != 0) {
      mShift.hardSet(index(mShift.value(), deltaShift));
    }

    mClockSwitch  = clockSwitch;
    mTrigSwitch   = trigSwitch;
    mResetSwitch  = resetSwitch;
    mRotateSwitch = rotateSwitch;
  }
}