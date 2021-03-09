#include <Or.h>
#include <od/extras/LookupTables.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/ops.h>

namespace lojik {
  Or::Or() {
    addInput(mLeft);
    addInput(mRight);
    addOutput(mOut);
  }

  Or::~Or() { }

  void Or::process() {
    float *left  = mLeft.buffer();
    float *right = mRight.buffer();
    float *out   = mOut.buffer();

    float32x4_t zero = vdupq_n_f32(0.0f);

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      float32x4_t l = vld1q_f32(left + i);
      float32x4_t r = vld1q_f32(right + i);

      // Add and check if the result is non-zero.
      float32x4_t m = vaddq_f32(l, r);
      vst1q_f32(out + i, vcvtq_n_f32_u32(vcgtq_f32(m, zero), 32));
    }
  }
}
// #include <Or.h>
// #include <od/extras/LookupTables.h>
// #include <od/constants.h>
// #include <od/config.h>
// #include <hal/ops.h>
// // #include <sstream>

// namespace lojik {
//   Or::Or(int n) {
//     if (n < 1) n = 1;

//     for (int i = 0; i < n; i++) {
//       switch (i) {
//         case 0: addInputFromHeap(new od::Inlet("In0")); break;
//         case 1: addInputFromHeap(new od::Inlet("In1")); break;
//         case 2: addInputFromHeap(new od::Inlet("In2")); break;
//         case 3: addInputFromHeap(new od::Inlet("In3")); break;
//         case 4: addInputFromHeap(new od::Inlet("In4")); break;
//       }
//     }

//     addOutput(mOut);
//   }

//   Or::~Or() { }

//   void Or::process() {
//     float *out = mOut.buffer();

//     float32x4_t zero = vdupq_n_f32(0.0f);
//     uint32_t n = mInputs.size();

//     for (int i = 0; i < FRAMELENGTH; i += 4) {
//       float32x4_t sum = vdupq_n_f32(0.0f);
//       for (uint32_t j = 0; j < n; j++) {
//         float *in = mInputs[j]->buffer();
//         float32x4_t v = vld1q_f32(in + i);
//         sum = vaddq_f32(sum, v);
//       }

//       vst1q_f32(out + i, vcvtq_n_f32_u32(vcgtq_f32(sum, zero), 32));
//     }
//   }
// }