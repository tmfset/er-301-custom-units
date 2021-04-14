#include <Sloop.h>
#include <SyncLatch.h>
#include <Slew.h>
#include <util.h>
#include <hal/simd.h>

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace sloop {
  void Sloop::process() {
    if (mpSample == 0) return;

    updateFades();

    Buffers buffers { *this };
    Constants constants { *this };

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      auto p = processInput(buffers, constants, i);
      processMonoToMono(buffers, constants, p, i);
    }
  }
}
