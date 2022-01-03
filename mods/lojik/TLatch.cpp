#include <TLatch.h>

namespace lojik {
  void TLatch::process() {
    processInternal();
  }
}