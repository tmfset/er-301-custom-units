#include <CPR.h>

namespace strike {
  void CPR::process() {
    switch (mChannelCount) {
      case 1: processInternal<1>(); break;
      case 2: processInternal<2>(); break;
    }
  }
}