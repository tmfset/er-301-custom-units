#pragma once

#include <math.h>
#include <hal/simd.h>
#include <hal/neon.h>
#include <util.h>
#include <shape.h>

namespace osc {
  namespace simd {
    struct Phase {
      inline float32x4_t envelope(
        const float32x4_t delta,
        const float32x4_t trig,
        const float32x4_t loop
      ) {
        bool _trig[4];
        trigger.readTriggers(_trig, trig);
        auto isLoop = util::simd::cgtqz_f32(loop);
        auto p = accumulate(phase, delta, _trig);
        p = vmlsq_f32(p, isLoop, util::simd::floor(p));
        p = vminq_f32(p, vdupq_n_f32(1));
        phase = vgetq_lane_f32(p, 3);
        return p;
      }

      inline float32x4_t oscillator(
        const float32x4_t delta,
        const float32x4_t sync
      ) {
        bool _sync[4];
        util::simd::cgt_as_bool(_sync, sync, vdupq_n_f32(0));
        auto p = accumulate(phase, delta, _sync);
        p = p - util::simd::floor(p);
        phase = vgetq_lane_f32(p, 3);
        return p;
      }

      inline float32x4_t accumulate(
        const float from,
        const float32x4_t delta,
        const bool *sync
      ) const {
        float _base[4], _scale[4], base = phase;
        for (int i = 0, s = 1; i < 4; i++, s++) {
          if (sync[i]) { base = 0; s = 0; }
          _base[i] = base;
          _scale[i] = s;
        }
        return vmlaq_f32(vld1q_f32(_base), delta, vld1q_f32(_scale));
      }

      util::latch trigger;
      float phase = 0.0f;
    };

    struct Fin {
      Phase phase;

      inline float32x4_t process(
        const float32x4_t freq,
        const float32x4_t width,
        const float32x4_t sync,
        const shape::simd::Bend &bend
      ) {
        auto delta = freq * vdupq_n_f32(globalConfig.samplePeriod);

        return shape::simd::fin(
          phase.oscillator(delta, sync),
          width,
          bend
        );
      }
    };
  }
}