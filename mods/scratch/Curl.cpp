#include <Curl.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/ops.h>
#include <hal/simd.h>
#include <math.h>

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace lojik {
  Curl::Curl() {
    addInput(mIn);
    addInput(mGain);
    addInput(mBias);
    addInput(mFold);
    addOutput(mOut);
  }

  Curl::~Curl() { }

  void Curl::process() {
    float *in   = mIn.buffer();
    float *gain = mGain.buffer();
    float *bias = mBias.buffer();
    float *fold = mFold.buffer();
    float *out  = mOut.buffer();

    for (int i = 0; i < FRAMELENGTH; i ++) {
      float x = (in[i] * gain[i]) + bias[i];
      float g = fold[i];
      out[i] = tanh(x) - (g * sin(x * 2.0f * (M_PI)));
    }
  }
}
