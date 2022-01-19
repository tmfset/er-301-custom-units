#include <ui/LinearDialMap.h>

namespace ui {
  LinearDialMap::LinearDialMap(
    float min,
    float max,
    float zero,
    float rounding,
    bool wrap,

    float sc,
    float c,
    float f,
    float sf
  ) :
    mRange(dial::Range(min, max)),
    mRadix(dial::Steps(sc, c, f, sf).proportionalRadixSet(mRange)),
    mRounding(rounding),
    mZero(zero),
    mWrap(wrap) { }

  LinearDialMap::~LinearDialMap() { }
}