#include <DLatch.h>

namespace lojik {
  void DLatch::process() {
    processInternal();
  }
}