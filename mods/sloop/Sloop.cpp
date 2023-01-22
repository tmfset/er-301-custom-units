#include <Sloop.h>
#include <SyncLatch.h>
#include <Slew.h>
#include <util/math.h>
#include <hal/simd.h>

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace sloop {
  void Sloop::process() {
    if (mpSample == 0) return;

    updateFades();
    updateManualResetStep(false);

    Buffers   buffers   { *this };
    Constants constants { *this };

    if (channels() < 2) {
      if (mpSample->mChannelCount < 2) {
        // Mono unit + mono sample
        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto p = processInput(buffers, constants, i);
          processResetOut(buffers, p, i);
          processMonoToMono(buffers, constants, p, i);
        }
      } else {
        // Mono unit + stereo sample
        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto p = processInput(buffers, constants, i);
          processResetOut(buffers, p, i);
          processMonoToStereo(buffers, constants, p, i);
        }
      }
    } else {
      if (mpSample->mChannelCount < 2) {
        // Stereo unit + mono sample
        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto p = processInput(buffers, constants, i);
          processResetOut(buffers, p, i);
          processStereoToMono(buffers, constants, p, i);
        }
      } else {
        // Stereo unit + stereo sample
        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto p = processInput(buffers, constants, i);
          processResetOut(buffers, p, i);
          processStereoToStereo(buffers, constants, p, i);
        }
      }
    }
  }
}
