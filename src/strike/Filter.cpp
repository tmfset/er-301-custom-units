#include <Filter.h>
#include <biquad.h>

namespace strike {
  void Filter::process() {
    switch (mMode.value()) {
      case STRIKE_FILTER_MODE_HIGHPASS:
        processType<biquad::HIGHPASS>();
        break;

      case STRIKE_FILTER_MODE_BANDPASS:
        processType<biquad::BANDPASS>();
        break;

      default:
        processType<biquad::LOWPASS>();
        break;
    }
  }
}