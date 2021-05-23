#include <LowPassGate.h>

namespace strike {
  void LowPassGate::process() {
    if (mChannelCount == 1) processChannels<1>();
    else                    processChannels<2>();
  }

  template <int CH>
  inline void LowPassGate::processChannels() {
    logInfo("LPG process");
    float *in[CH], *out[CH];
    svf::simd::Filter *filter[CH];
    for (int channel = 0; channel < CH; channel++) {
      in[channel]  = getInput(channel)->buffer();
      out[channel] = getOutput(channel)->buffer();
      filter[channel] = &mFilter.at(channel);
    }

    const float *trig   = mTrig.buffer();
    const float *loop   = mLoop.buffer();
    const float *rise   = mRise.buffer();
    const float *fall   = mFall.buffer();
    const float *bend   = mBend.buffer();
    const float *height = mHeight.buffer();

    shape::BendMode bendMode = (shape::BendMode)mBendMode.value();

    env::simd::Frequency f;
    svf::simd::Coefficients cf;
    svf::simd::Coefficients ecf;

    ecf.updateLowpass(
      vdupq_n_f32(200),
      vdupq_n_f32(0.70710678118f)
    );

    logInfo("LPG process loop");

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      f.setRiseFall(vld1q_f32(rise + i), vld1q_f32(fall + i));

      auto e = mEnvelope.process(
        vld1q_f32(trig + i),
        vld1q_f32(loop + i),
        f,
        shape::simd::Bend { bendMode, vld1q_f32(bend + i), }
      );

      auto ef = mEnvelopeFilter.processLowpass(ecf, e);

      auto h = vld1q_f32(height + i);

      cf.updateLowpass(
        util::simd::vpo_scale(ef * h, vdupq_n_f32(100)),
        vdupq_n_f32(0.70710678118f)
      );

      for (int c = 0; c < CH; c++) {
        const auto _in = vld1q_f32(in[c] + i);
        const auto _out = util::simd::tanh(filter[c]->processLowpass(cf, _in));
        vst1q_f32(out[c] + i, _out * ef);
        //vst1q_f32(out[c] + i, e);
      }
    }
  }
}