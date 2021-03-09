#include <Register.h>
#include <od/extras/LookupTables.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/ops.h>

namespace lojik {
  Register::Register() {
    addInput(mIn);
    addOutput(mOut);

    addInput(mLength);
    addInput(mOrigin);

    addInput(mCapture);
    addInput(mShift);
    addInput(mStep);

    addInput(mZero);
    addInput(mZeroAll);
    addInput(mRandomize);
    addInput(mRandomizeAll);

    mData = new float[128];
  }

  Register::~Register() {
    delete [] mData;
  }

  void Register::process() {
    
    float *in       = mIn.buffer();
    float *out      = mOut.buffer();

    // for (int i = 0; i < FRAMELENGTH; i ++) {
    //   uint32_t value = static_cast<uint32_t>(in[0]);
    //   out[i] = mData[10] + static_cast<float>(value % 10);
    // }

    float *length   = mLength.buffer();
    float *origin   = mOrigin.buffer();

    float *capture  = mCapture.buffer();
    float *shift    = mShift.buffer();
    float *step     = mStep.buffer();

    float *zero     = mZero.buffer();
    float *zeroAll  = mZeroAll.buffer();
    float *rand     = mRandomize.buffer();
    float *randAll  = mRandomizeAll.buffer();
    float *randGain = mRandomizeGain.buffer();

    for (int i = 0; i < FRAMELENGTH; i ++) {
      uint32_t maxSteps = 8;//CLAMP(1, 128, static_cast<int>(length[i]));

      if (step[i] <= 0.5f) {
        canStep = true;
      }

      if (canStep && step[i] > 0.5f) {
        mIndex = (mIndex + 1) % maxSteps;
        canStep = false;
        canCapture = true;
      }

      if (shift[i] <= 0.5f) {
        canShift = true;
      }

      if (canShift && shift[i] > 0.5f) {
        mOffset = (mOffset + 1) % maxSteps;
        canShift = false;
        canCapture = true;
      }

      if (origin[i] <= 0.5f) {
        canOrigin = true;
      }

      if (canOrigin && origin[i] > 0.5f) {
        mIndex = 0;
        canOrigin = false;
      }

      uint32_t index = (mIndex + mOffset) % maxSteps;

      if (canCapture && capture[i] > 0.5f) {
        mData[index] = in[i];
        canCapture = false;
      }
      
      if (zero[i] > 0.5f) {
        mData[index] = 0.0f;
      }

      if (zeroAll[i] > 0.5f) {
        for (uint32_t j = 0; j < maxSteps; j++) {
          mData[j] = 0.0f;
        }
      }

      // if (rand[i] > 0.5f) {
      //   float r = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
      //   float v = ((r * 2) - 1) * randGain[i];
      //   mData[mVirtualStep] = v;
      // }

      // if (randAll[i] > 0.5f) {
      //   for (uint32_t j = 0; j < maxSteps; j++) {
      //     float r = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
      //     float v = ((r * 2) - 1) * randGain[i];
      //     mData[j] = v;
      //   }
      // }

      out[i] = mData[index];
    }
  }
}