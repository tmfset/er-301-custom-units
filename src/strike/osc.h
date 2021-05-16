#pragma once

#include <math.h>
#include <hal/simd.h>
#include <hal/neon.h>
#include <util.h>

namespace osc {
  namespace simd {
    struct Frequency {
      inline void update(const float32x4_t f) {
        phaseDelta = f * samplePeriod;
      }

      const float32x4_t samplePeriod = vdupq_n_f32(globalConfig.samplePeriod);
      float32x4_t phaseDelta = vdupq_n_f32(0);
    };

    struct Oscillator {
      inline float32x4_t process(
        const Frequency &cf,
        const uint32x4_t sync
      ) {
        uint32_t _sync[4];
        vst1q_u32(_sync, sync);

        float _phase[4], _factor[4], base = phase;
        for (int i = 0, x = 1; i < 4; i++, x++) {
          if (_sync[i]) { base = 0; x = 0; }
          _phase[i]  = base;
          _factor[i] = x;
        }

        float32x4_t p = vld1q_f32(_phase);
        float32x4_t f = vld1q_f32(_factor);
        p = vmlaq_f32(p, f, cf.phaseDelta);
        p = p - util::simd::floor(p);

        float32x4_t wrap = p - vdupq_n_f32(1);
        float32x4_t mask = vcvtq_n_f32_u32(vcltq_f32(wrap, vdupq_n_f32(0)), 32);
        p = vmaxq_f32(p * mask, wrap);

        phase = vgetq_lane_f32(p, 3);

        return p;
      }

      float phase = 0.0f;
    };
  }
}