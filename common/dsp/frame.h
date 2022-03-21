#pragma once

#include <od/config.h>
#include <od/constants.h>

namespace dsp {
  namespace frame {
    inline uint32_t anyOverThreshold(const float *frame, float threshold) {
      auto _out       = vdupq_n_u32(0);
      auto _threshold = vdupq_n_f32(threshold);

      for (int i = 0; i < FRAMELENGTH; i += 4) {
        auto over = vcgtq_f32(vld1q_f32(frame + i), _threshold);
        _out = vorrq_u32(_out, over);
      }

      auto t = vpmax_u32(vget_low_u32(_out), vget_high_u32(_out));
      return vget_lane_u32(vpmax_u32(t, t), 0);
    }
  }
}