#pragma once

#include <Register.h>
#include <od/graphics/Graphic.h>
#include <math.h>

namespace lojik {
  class RegisterMainView : public od::Graphic {
    public:
      RegisterMainView(int left, int bottom, int width, int height) : od::Graphic(left, bottom, width, height) { }
      virtual ~RegisterMainView() {
        if (mpRegister) mpRegister->release();
      }

#ifndef SWIGLUA
      virtual void draw(od::FrameBuffer &fb) {
        if (!mpRegister) return;

        // auto pad = 6;
        // auto innerWidth = mWidth - pad * 2;
        // auto innerHeight = mHeight - pad * 2;











        auto xCount = 8.0f;
        auto yCount = 8.0f;

        auto pad = 5;

        auto xSize = (mWidth - pad * 2) / xCount;
        auto ySize = (mHeight - pad * 2) / yCount;

        auto boxPad = 2;
        int size = (xSize < ySize ? xSize : ySize) - 2;

        auto top = mWorldBottom + mHeight - pad;

        auto state = mpRegister->state();
        float max = -1;
        float min = 1;

        for (int i = 0; i < state.max(); i++) {
          auto v = state.getRaw(i);
          max = v > max ? v : max;
          min = v < min ? v : min;
        }

        float scale = max - min;

        for (int i = 0; i < xCount; i++) {
          auto x = mWorldLeft + pad + i * (size + boxPad);
          for (int j = 0; j < yCount; j++) {
            auto y = top - (j + 1) * (size + boxPad);
            auto v = state.getRaw(i + xCount * j);
            auto color = GRAY10 * (scale - (max - v)) / scale;
            fb.fill((int)color, x, y, x + size, y + size);
          }
        }

        //fb.fill(WHITE, mWorldLeft, mWorldBottom, mWorldLeft + 20, mWorldBottom + 20);
      }
#endif

      void follow(Register *pRegister) {
        if (mpRegister) mpRegister->release();
        mpRegister = pRegister;
        if (mpRegister) mpRegister->attach();
      }

    private:
      Register *mpRegister = 0;
  };
}