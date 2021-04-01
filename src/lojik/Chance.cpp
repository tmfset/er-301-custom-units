#include <Chance.h>
#include <od/constants.h>
#include <od/config.h>
#include <od/extras/Random.h>
#include <hal/ops.h>
#include <hal/simd.h>
#include <sense.h>

namespace lojik {
  Chance::Chance() {
    addInput(mIn);
    addInput(mChance);
    addOutput(mOut);

    addOption(mMode);
    addOption(mSense);
  }

  Chance::~Chance() { }

  void Chance::process() {
    float *in     = mIn.buffer();
    float *chance = mChance.buffer();
    float *out    = mOut.buffer();

    float32x4_t sense = vdupq_n_f32(getSense(mSense));

    int mode = mMode.value();

    OneTime trigSwitch { mTrigSwitch, false };
    bool allow = mAllow;

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      float32x4_t loadIn     = vld1q_f32(in + i);
      float32x4_t loadChance = vld1q_f32(chance + i);

      uint32_t isInHigh[4];
      vst1q_u32(isInHigh, vcgtq_f32(loadIn, sense));

      for (int j = 0; j < 4; j++) {
        trigSwitch.mark(isInHigh[j]);
        bool doCapture = trigSwitch.read();

        if (doCapture) {
          allow = loadChance[j] > od::Random::generateFloat(0.0f, 1.0f);
        }

        float value = 0.0f;
        if (allow) {
          switch (mode) {
            case CHANCE_MODE_TRIGGER:
              value = doCapture ? 1.0f : 0.0f;
              break;

            case CHANCE_MODE_GATE:
              value = isInHigh[j] ? 1.0f : 0.0f;
              break;

            case CHANCE_MODE_PASSTHROUGH:
              value = loadIn[j];
              break;
          }
        }

        out[i + j] = value;
      }
    }

    mTrigSwitch = trigSwitch;
    mAllow = allow;
  }
}