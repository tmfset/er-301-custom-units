#pragma once

#include <math.h>
#include <hal/simd.h>
#include <util.h>
#include <shape.h>
#include <osc.h>

namespace env {
  namespace simd {
    struct Frequency {
      void setRiseFall(
        const float32x4_t _rise,
        const float32x4_t _fall
      ) {
        auto period = _rise + _fall;
        auto oneOverPeriod = util::simd::invert(period);
        delta = oneOverPeriod * vdupq_n_f32(globalConfig.samplePeriod);
        width = oneOverPeriod * _rise;
      }

      void setPeriodWidth(
        const float32x4_t _period,
        const float32x4_t _width
      ) {
        auto oneOverPeriod = util::simd::invert(_period);
        delta = oneOverPeriod * vdupq_n_f32(globalConfig.samplePeriod);
        width = _width;
      }

      float32x4_t delta, width;
    };

    struct AD {
      osc::simd::Phase phase;

      inline float32x4_t process(
        const float32x4_t trig,
        const float32x4_t loop,
        const Frequency& freq,
        const shape::simd::Bend &bend
      ) {
        return shape::simd::fin(
          phase.envelope(freq.delta, trig, loop),
          freq.width,
          bend
        );
      }
    };
  }
}