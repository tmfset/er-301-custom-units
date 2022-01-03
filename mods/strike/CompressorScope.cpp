#include "CompressorScope.h"

namespace strike {
  CompressorScope::CompressorScope(
    int left,
    int bottom,
    int width,
    int height
  ) : od::Graphic(left, bottom, width, height),
      mScopeExcite(left, bottom, width, height, WHITE, true),
      mScopeReduction(left, bottom, width, height, WHITE, false) {
    own(mScopeExcite);
    addChild(&mScopeExcite);

    own(mScopeReduction);
    addChild(&mScopeReduction);
  }
}