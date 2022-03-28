#pragma once

#include <od/graphics/FrameBuffer.h>

#include <graphics/composites/Text.h>
#include <graphics/interfaces/all.h>
#include <graphics/primitives/all.h>

namespace graphics {
  struct Keyboard {
    //  1 3   6 8 A
    // 0 2 4 5 7 9 B
    static inline bool isBlackKey(int key) {
      if (key == 1) return true;
      if (key == 3) return true;
      if (key == 6) return true;
      if (key == 8) return true;
      if (key == 10) return true;
      return false;
    }

    static inline v2d keyPosition(const v2d &runRise, float key) {
      float offset = key > 4.0f ? 1.0f : 0.0f;
      return runRise * v2d::of(key + offset, isBlackKey(util::fhr(key)) ? 1.0f : 0.0f);
    }

    static inline v2d keyPositionInt(const v2d &runRise, int key) {
      float offset = key > 4.0f ? 1.0f : 0.0f;
      return runRise * v2d::of((float)key + offset, isBlackKey(key) ? 1.0f : 0.0f);
    }

    static inline v2d keyPositionInterpolate(const v2d &runRise, float key, bool vertical) {
      // 2.3  -> 2, 3
      // 10.2 -> 10, 11
      // 11.2 -> 11, 12
      // 11.6 -> -1, 0
      key = key > 11.5 ? key - 12.0f : key;
      auto keyLow = util::fdr(key);
      auto keyHigh = util::fur(key);
      auto low = keyPositionInt(runRise, keyLow);
      auto high = keyPositionInt(runRise, keyHigh);

      auto result = low.lerp(high, key - keyLow);
      return vertical ? result.swap() : result;
    }

    static Circle scale(const Circle &base, float value) {
      float amount = util::flerpi(0.7f, 1.0f, util::fabs(value - (util::fdr(value) + 0.5)) * 2.0f);
      return base.scale(amount);
    }

    static inline void draw(
      od::FrameBuffer &fb,
      HasScale &data,
      const Circle &base,
      const v2d &runRise,
      bool vertical = false
    ) {
      auto size      = data.getScaleSize();
      auto isEmpty   = size == 0;

      auto detected  = data.getDetectedCentValue() / 100.0f;
      base.offset(keyPositionInterpolate(runRise, detected, vertical)).fill(fb, GRAY3);

      if (!isEmpty) {
        auto quantized = data.getQuantizedCentValue() / 100.0f;
        scale(base, quantized).offset(keyPositionInterpolate(runRise, quantized, vertical)).fill(fb, GRAY10);
      }

      for (int i = 0; i < 12; i++) {
        base.offset(keyPositionInterpolate(runRise, i, vertical)).trace(fb, GRAY5);
      }

      for (int i = 0; i < size; i++) {
        float key = data.getScaleCentValue(i) / 100.0f;
        scale(base, key).offset(keyPositionInterpolate(runRise, key, vertical)).trace(fb, GRAY13);
      }
    }
  };

  struct HKeyboard {
    inline HKeyboard(HasScale &data) :
        mScaleData(data) {
      mScaleData.attach();
    }

    inline ~HKeyboard() {
      mScaleData.release();
    }

    inline void draw(od::FrameBuffer &fb, const Box &world, float scale) const {
      auto inner   = world.padY(2);

      auto key     = inner.segmentQSquare(7, 2);
      auto runRise = key.widthHeight().scaleX(0.5f).quantize();

      auto bounds = Box::wh(runRise.scale(13, 2)).recenterOn(world);
      auto base   = key.atLeftBottom(bounds).minCircle().scale(scale);

      Keyboard::draw(fb, mScaleData, base, runRise);

      auto octave = mScaleData.getDetectedOctaveValue();
      //auto offset = runRise.atY(0).scale(1.5).rotateCW();
      auto arrowBase = base.center.atY(world.bottom()).offsetY(-3);
      if (octave < 0) Point(arrowBase.offset(Keyboard::keyPosition(runRise, 0))).arrowsLeft(fb, GRAY10, -octave, 3);
      if (octave > 0) Point(arrowBase.offset(Keyboard::keyPosition(runRise, 11))).arrowsRight(fb, GRAY10, octave, 3);

      auto size = mScaleData.getScaleSize();
      auto color = size == 0 ? GRAY5 : GRAY13;

      auto text = Text(mScaleData.getScaleName(), 6);
      text.draw(fb, color, world.outBottom(8), CENTER_TOP);
    }

    HasScale &mScaleData;
  };

  struct IKeyboard {
    inline IKeyboard(HasScale &data) :
        mScaleData(data) {
      mScaleData.attach();
    }

    inline ~IKeyboard() {
      mScaleData.release();
    }
    
    inline void draw(od::FrameBuffer &fb, const Box &world, float scale) const {
      auto inner = world.padY(2 + 1);//.padBottom(8 + 2).padTop(2 + 1).offsetY(-1);

      auto key     = inner.segmentQSquare(2, 7);
      auto runRise = key.widthHeight().scale(0.5f, -1).quantize();

      auto bounds = Box::wh(runRise.scale(14, 2).swap()).recenterOn(inner);
      auto base   = key.atRightBottom(bounds).minCircle().scale(scale);

      //key.atRightBottom(bounds).trace(fb, WHITE);
      //bounds.trace(fb, WHITE);

      Keyboard::draw(fb, mScaleData, base, runRise, true);

      auto octave = mScaleData.getDetectedOctaveValue();
      auto arrowBase = base.center.atX(bounds.right()).offsetX(3);
      if (octave < 0) Point(arrowBase.offset(Keyboard::keyPosition(runRise, 0).swap())).arrowsDown(fb, GRAY10, -octave, 3, 0.9);
      if (octave > 0) Point(arrowBase.offset(Keyboard::keyPosition(runRise, 11).swap())).arrowsUp(fb, GRAY10, octave, 3, 0.9);

      // auto size = mScaleData.getScaleSize();
      // auto color = size == 0 ? GRAY5 : GRAY13;

      // auto text = Text(mScaleData.getScaleName(), 5);
      // text.setJustifyAlign(RIGHT_MIDDLE);
      // text.draw(fb, color, bounds.fromRight(10), true);
    }

    HasScale &mScaleData;
    float mPad;
  };
}