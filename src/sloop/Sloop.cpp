#include <Sloop.h>
#include <SyncLatch.h>
#include <Slew.h>
#include <util.h>
#include <hal/simd.h>

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace sloop {
  void Sloop::process() {
    if (mpSample == 0) return;

    FrameConst frame {
      mClock.buffer(),
      mEngage.buffer(),
      mRecord.buffer(),
      mOverdub.buffer(),
      mReset.buffer(),
      mLength.buffer(),
      mRLength.buffer(),

      mResetFlags.getFlag(0),
      mResetFlags.getFlag(1),

      vdupq_n_f32(0),
      vdupq_n_f32(1),

      vdupq_n_f32(mFeedback.value()),
      vdupq_n_f32(mThrough.value()),
      (int)mResetTo.value()
    };

    mFadeSlew = Slew {
      mFadeSlew,
      globalConfig.samplePeriod / fclampMin(mFadeIn.value(), globalConfig.samplePeriod),
      globalConfig.samplePeriod / fclampMin(mFadeOut.value(), globalConfig.samplePeriod)
    };

    mShadowSlew = Slew {
      mShadowSlew,
      1,
      globalConfig.samplePeriod / fclampMin(mFade.value(), globalConfig.samplePeriod)
    };

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      ProcessInput input;
      auto processed = input.process(*this, frame, i);

      ProcessOutput output;
      output.process(*this, frame, processed, i);
    }
  }

  // void write(float *out, float *in) {

  // }
}

/*
        if (!engaged) {
          for (int channel = 0; channel < channels; channel++) {
            out[channel][i + j] = 0.0f;
          }

          continue;
        }

        float inptLevel = fadeSlew.move(record || overdub ? 1.0f : 0.0f);
        float shdwLevel = shadowSlew.move(0.0f);
        float fdbkLevel = 1.0f + (feedback - 1.0f) * inptLevel;

        if (channels < 2) {
          if (sampleChannels < 2) {
            float input   = in[0][i + j];
            float current = mpSample->get(mCurrentIndex, 0);
            float shadow  = mpSample->get(mShadowIndex, 0);

            mpSample->set(mShadowIndex, 0, flerp(
              shdwLevel,
              inptLevel * input + fdbkLevel * shadow,
              shadow
            ));

            mpSample->set(mCurrentIndex, 0, inptLevel * input + fdbkLevel * current);

          } else {
            mpSample->set(mCurrentIndex, 0, loadIn[0][j]);
            mpSample->set(mCurrentIndex, 1, loadIn[0][j]);
          }
        } else {
          if (sampleChannels < 2) {
            float value = (loadIn[0][j] + loadIn[1][j]) * 0.5f;
            mpSample->set(mCurrentIndex, 0, value);
          } else {
            mpSample->set(mCurrentIndex, 0, loadIn[0][j]);
            mpSample->set(mCurrentIndex, 1, loadIn[1][j]);
          }
        }

        if (channels < 2) {
          if (sampleChannels < 2) {
            float current = mpSample->get(mCurrentIndex, 0);
            float shadow  = mpSample->get(mShadowIndex, 0);
            out[0][i + j] = flerp(shdwLevel, shadow, current);
            //out[0][i + j] = current;
          } else {
            float left  = mpSample->get(mCurrentIndex, 0);
            float right = mpSample->get(mCurrentIndex, 1);
            out[0][i + j] = (left + right) * 0.5f;
          }
        } else {
          if (sampleChannels < 2) {
            out[0][i + j] = mpSample->get(mCurrentIndex, 0);
            out[1][i + j] = out[0][i + j];
          } else {
            out[0][i + j] = mpSample->get(mCurrentIndex, 0);
            out[1][i + j] = mpSample->get(mCurrentIndex, 1);
          }
        }
*/