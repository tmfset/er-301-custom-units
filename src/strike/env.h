#pragma once

#include <math.h>
#include <hal/simd.h>
#include <hal/neon.h>
#include <util.h>

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace env {
  struct Latch {
    inline bool readTrigger(bool high) {
      mTrigger = mEnable && high;
      mEnable  = !mTrigger && !high;
      return mTrigger;
    }

    inline void readTriggers(bool *out, const float32x4_t high) {
      uint32_t _h[4];
      vst1q_u32(_h, vcgtq_f32(high, vdupq_n_f32(0)));
      bool _t = mTrigger;
      bool _e = mEnable;
      for (int i = 0; i < 4; i++) {
        bool h = _h[i];
        _t = _e && h;
        _e = !_t && !h;
        out[i] = _t;
      }
      mTrigger = _t;
      mEnable = _e;
    }

    bool mEnable  = true;
    bool mTrigger = false;
  };

  namespace simd {

    struct Frequency {

      inline void update(
        const float32x4_t rise,
        const float32x4_t fall
      ) {
        auto period = rise + fall;
        auto oneOverPeriod = util::simd::invert(period);

        phaseDelta = oneOverPeriod * samplePeriod;
        width = oneOverPeriod * rise;
      }

      const float32x4_t samplePeriod = vdupq_n_f32(globalConfig.samplePeriod);

      float32x4_t phaseDelta;
      float32x4_t width;
      float32x4_t oneOverWidth;
      float32x4_t oneOverWidthInv;
    };

    struct AD {
      inline float32x4_t process(
        const Frequency& cf,
        const float32x4_t trig,
        const float32x4_t loop,
        const float32x4_t bendUp,
        const float32x4_t bendDown
      ) {
        auto zero = vdupq_n_f32(0);
        auto isLoopHigh = vcvtq_n_f32_u32(vcgtq_f32(loop, zero), 32);

        bool _it[4];
        trigger.readTriggers(_it, trig);

        float _b[4], _s[4], base = phase;
        int x = 0;
        for (int i = 0; i < 4; i++) {
          bool it = _it[i];
          base = it ? 0 : base;
          x = it ? 0 : x + 1;

          _b[i] = base;
          _s[i] = x;
        }

        auto one = vdupq_n_f32(1);

        float32x4_t _p = vld1q_f32(_b);
        float32x4_t scale = vld1q_f32(_s);
        _p = vmlaq_f32(_p, cf.phaseDelta, scale);
        _p = vmlsq_f32(_p, isLoopHigh, vcvtq_f32_s32(vcvtq_s32_f32(_p)));
        _p = vminq_f32(_p, one);
        phase = vgetq_lane_f32(_p, 3);

        rising  = vcvtq_n_f32_u32(vcaleq_f32(_p, cf.width), 32);
        falling = one - rising;

        auto t = vmlaq_f32(falling, cf.width, rising - falling);
        auto tInv = util::simd::invert(t);

        auto distance = vabdq_f32(_p, cf.width);
        auto linear = vmlsq_f32(one, distance, tInv);
        
        return linear;
      }

      Latch trigger;

      float32x4_t rising;
      float32x4_t falling;
      float phase = 1.0f;
    };
  }

  struct AD {
    inline float process(
      const float rise,
      const float fall,
      const bool trig,
      const bool loop,
      const float bend
    ) {
      if (trig || loop) hold = false;

      float period = rise + fall;
      float phaseDelta = 0.0f;
      if (!hold) phaseDelta = globalConfig.samplePeriod / period;

      phase += phaseDelta;

      if (phase >= 1.0f) {
        if (loop) {
          phase -= (int)phase;
        } else {
          hold = true;
          phase = 0.0f;
        }
      }

      float riseLength = rise / period;
      float fallLength = fall / period;

      float linear = 0.0f;

      if (phase < riseLength) linear = phase / riseLength;
      else linear = (fallLength - (phase - riseLength)) / fallLength;

      util::exp_scale b { 0.001f, 1.0f };
      util::exp_scale s { b.process(1.0f - bend), 1 };

      return s.processBase(linear);
    }

    bool hold = false;
    float phase = 0;
  };
}