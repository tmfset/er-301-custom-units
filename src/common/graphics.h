#pragma once

#include <util.h>

namespace graphics {
  struct Point {
    inline Point(float _x, float _y) :
      x(_x),
      y(_y) { }

    inline Point offsetX(float byX) {
      return Point(x + byX, y);
    }

    inline Point offsetY(float byY) {
      return Point(x, y + byY);
    }

    inline Point atX(float _x) const {
      return Point(_x, y);
    }

    inline Point atY(float _y) const {
      return Point(x, _y);
    }

    inline Point quantize() const {
      return Point(util::fhr(x), util::fhr(y));
    }

    inline void circle(od::FrameBuffer &fb, od::Color color, float radius) const {
      fb.circle(color, x, y, radius);
    }

    inline void fillCircle(od::FrameBuffer &fb, od::Color color, float radius) const {
      fb.fillCircle(color, x, y, radius);
    }

    inline void diamond(od::FrameBuffer &fb, od::Color color) const {
      fb.pixel(color, x - 1, y);
      fb.pixel(color, x + 1, y);
      fb.pixel(color, x, y - 1);
      fb.pixel(color, x, y + 1);
    }

    inline void dot(od::FrameBuffer &fb, od::Color color) const {
      fb.pixel(color, x, y);
    }

    const float x;
    const float y;
  };

  struct Box {
    inline Box(float _left, float _bottom, float _right, float _top) :
        left(_left),
        bottom(_bottom),
        right(_right),
        top(_top),
        width(_right - _left),
        height(_top - _bottom),
        center(Point(_left + width * 0.5f, _bottom + height * 0.5f)) { }

    static inline Box lbrt_raw(float l, float b, float r, float t) {
      return Box(l, b, r, t);
    }

    static inline Box lbrt(float _left, float _bottom, float _right, float _top) {
      auto l = util::fmin(_left, _right);
      auto b = util::fmin(_bottom, _top);
      auto r = util::fmax(_left, _right);
      auto t = util::fmax(_bottom, _top);
      return lbrt_raw(l, b, r, t);
    }

    static inline Box lbwh_raw(float l, float b, float w, float h) {
      return lbrt_raw(l, b, l + w, b + h);
    }

    static inline Box lbwh(float _left, float _bottom, float _width, float _height) {
      return lbrt(_left, _bottom, _left + _width, _bottom + _height);
    }

    static inline Box cwr(float _x, float _y, float _width, float _rise) {
      auto hWidth = _width / 2.0f;
      return lbrt(_x - hWidth, _y, _x + hWidth, _y + _rise);
    }

    static inline Box wh(float _width, float _height) {
      return lbwh(0, 0, _width, _height);
    }

    static inline Box cwh(const Point &c, float _width, float _height) {
      auto hWidth = util::fabs(_width) / 2.0f;
      auto hHeight = util::fabs(_height) / 2.0f;
      return lbrt_raw(c.x - hWidth, c.y - hHeight, c.x + hWidth, c.y + hHeight);
    }

    static inline Box cs(const Point &c, float _size) {
      return cwh(c, _size, _size);
    }

    inline Box inner(float margin) const {
      return inner(margin, margin);
    }

    inline Box innerX(float marginX) const {
      return lbrt(
        left + marginX,
        bottom,
        right - marginX,
        top
      );
    }

    inline Box innerY(float marginY) const {
      return lbrt(
        left,
        bottom + marginY,
        right,
        top - marginY
      );
    }

    inline Box inner(float marginX, float marginY) const {
      return lbrt(
        left   + marginX,
        bottom + marginY,
        right  - marginX,
        top    - marginY
      );
    }

    inline Box square(float size) const {
      return cs(center, size);
    }

    inline Box padRight(float by) const {
      return lbrt(left, bottom, right - by, top);
    }

    inline Box topLeftCorner(float w, float h) const {
      return lbrt_raw(left, top - height * h, left + width * w, top);
    }

    inline Box divideTop(float by) const {
      return lbrt_raw(left, top - height * by, right, top);
    }

    inline Box divideBottom(float by) const {
      return lbrt_raw(left, bottom, right, bottom + height * by);
    }

    inline Box splitLeft(float by) const {
      return lbrt_raw(left, bottom, left + width * by, top);
    }

    inline Box splitRight(float by) const {
      return lbrt_raw(right - width * by, bottom, right, top);
    }

    inline Box withWidthFromLeft(float _width) const {
      return lbwh(left, bottom, _width, height);
    }

    inline Box offset(float byX, float byY) const {
      return lbrt_raw(left + byX, bottom + byY, right + byX, top + byY);
    }

    inline Box offsetX(float by) const {
      return lbrt_raw(left + by, bottom, right + by, top);
    }

    inline Box offsetY(float by) const {
      return lbrt_raw(left, bottom + by, right, top + by);
    }

    inline Box quantizeSize() const {
      return cwh(center, util::fhr(width), util::fhr(height));
    }

    inline Box quantizeCenter() const {
      return cwh(center.quantize(), width, height);
    }

    inline Box quantize() const {
      return quantizeSize().quantizeCenter();
    }

    inline Box intersect(const Box& other) const {
      return lbrt(
        util::fmax(left, other.left),
        util::fmax(bottom, other.bottom),
        util::fmin(right, other.right),
        util::fmin(top, other.top)
      );
    }

    inline Point rightCenter() const {
      return center.atX(right);
    }

    inline Box recenter(const Point &c) const {
      return cwh(c, width, height);
    }

    inline Box recenterOn(const Box& other) const {
      return recenter(other.center);
    }

    inline Box recenterY(float y) const {
      return recenter(center.atY(y));
    }

    inline Box recenterX(float x) const {
      return recenter(center.atX(x));
    }

    inline Box insert(const Box& other) const {
      auto l = util::fclamp(other.left, left, clampX(right - other.width));
      auto b = util::fclamp(other.bottom, bottom, clampY(top - other.height));
      auto r = util::fclamp(other.right, clampX(l + other.width), right);
      auto t = util::fclamp(other.top, clampY(b + other.height), top);
      return lbrt(l, b, r, t);
    }

    inline float clampX(float x) const {
      return util::fclamp(x, left, right);
    }

    inline float clampY(float y) const {
      return util::fclamp(y, bottom, top);
    }

    inline float minDimension() const {
      return util::fmin(width, height);
    }

    inline Box minSquare() const {
      return cs(center, minDimension());
    }

    inline Box scaleDiscrete(int w, int h) const {
      return cwh(center, width * w, height * h);
    }

    inline bool containsX(float x) const {
      return x > left && x < right;
    }

    inline void fill(od::FrameBuffer &fb, od::Color color) const {
      fb.fill(color, left, bottom, right, top);
    }

    inline void clear(od::FrameBuffer &fb) const {
      fb.clear(left, bottom, right, top);
    }

    inline void line(od::FrameBuffer &fb, od::Color color) const {
      fb.box(color, left, bottom, right, top);
    }

    inline void outline(od::FrameBuffer &fb, od::Color color) const {
      fb.box(color, (int)left - 1, (int)bottom - 1, (int)right + 1, (int)top + 1);
    }

    inline void circle(od::FrameBuffer &fb, od::Color color) const {
      center.circle(fb, color, width / 2.0f);
    }

    inline void fillCircle(od::FrameBuffer &fb, od::Color color) const {
      center.fillCircle(fb, color, width / 2.0f);
    }

    const float left;
    const float bottom;
    const float right;
    const float top;

    const float width;
    const float height;

    const Point center;
  };

  struct Grid {
    inline Grid(const Box &topLeft, int _cols, int _rows, float pad) :
      mark(topLeft.quantize().inner(pad)),
      cols(_cols),
      rows(_rows),
      cStep(topLeft.width),
      rStep(-topLeft.height) { }

    static inline Grid create(const Box& world, int cols, int rows, float pad) {
      float iCols = 1.0f / cols;
      float iRows = 1.0f / rows;

      auto corner = world.topLeftCorner(iCols, iRows).minSquare().quantizeSize();
      auto grid = corner.scaleDiscrete(cols, rows).recenterOn(world).quantizeCenter();

      return Grid(grid.topLeftCorner(iCols, iRows), cols, rows, pad);
    }

    static inline Grid createRect(const Box& world, int cols, int rows, float pad) {
      float iCols = 1.0f / cols;
      float iRows = 1.0f / rows;

      auto corner = world.topLeftCorner(iCols, iRows).quantizeSize();
      auto grid = corner.scaleDiscrete(cols, rows).recenterOn(world).quantizeCenter();

      return Grid(grid.topLeftCorner(iCols, iRows), cols, rows, pad);
    }

    inline int index(int c, int r) const {
      return c * rows + r;
    }

    inline Box cell(int c, int r) const {
      return mark.offset(cStep * c, rStep * r);
    }

    const Box mark;
    const int cols;
    const int rows;
    const float cStep;
    const float rStep;
  };

  struct IFader {
    inline IFader(float _x, float _bottom, float _top, float _width) :
      x(_x),
      span(2),
      width(_width),
      height(_top - _bottom),
      bottom(_bottom),
      center(_bottom + height / 2.0f),
      top(_top) { }

    static inline IFader cwh(const Point &c, float w, float h) {
      auto w2 = w / 2.0f;
      auto h2 = h / 2.0f;
      return IFader(c.x, c.y - h2, c.y + h2, w2);
    }

    static inline IFader box(const Box &b) {
      return IFader(b.center.x, b.bottom, b.top, b.width);
    }

    inline void draw(od::FrameBuffer &fb, od::Color color) const {
      fb.vline(color, x, bottom, top);
      fb.hline(color, x - span, x + span, bottom);
      fb.hline(color, x - span, x + span, top);
    }

    inline float drawActual(od::FrameBuffer &fb, od::Color color, float value) const {
      auto y = center + (height / 2.0f) * value;
      fb.hline(color, x - (span + 2), x + (span + 2), y);
      return y;
    }

    inline float drawTarget(od::FrameBuffer &fb, od::Color color, float value) const {
      auto y = center + (height / 2.0f) * value;
      fb.box(color, x - (span + 1), y - 1, x + (span + 1), y + 1);
      return y;
    }

    const float x;

    const int span;
    const float width;
    const float height;

    const float bottom;
    const float center;
    const float top;
  };

  struct HFader {
    inline HFader(float _y, float _left, float _right, float _height) :
      y(_y),
      hSpan(1),
      tSpan(util::fclamp(_height / 2.0f, hSpan, hSpan + 1)),
      aSpan(util::fclamp(_height / 2.0f, tSpan, tSpan + 1)),
      width(_right - _left),
      height(_height),
      left(_left),
      center(_left + width / 2.0f),
      right(_right) { }

    static inline HFader cwh(const Point &c, float w, float h) {
      auto w2 = w / 2.0f;
      auto h2 = h / 2.0f;
      return HFader(c.y, c.x - w2, c.x + w2, h2);
    }

    static inline HFader box(const Box &b) {
      return HFader(b.center.y, b.left, b.right, b.height / 2.0f);
    }

    inline void draw(od::FrameBuffer &fb, od::Color color) const {
      fb.hline(color, left, right, y);
      fb.vline(color, left, y - hSpan, y);
      fb.vline(color, right, y, y + hSpan);
    }

    inline float drawActual(od::FrameBuffer &fb, od::Color color, float value) const {
      auto x = util::fhr(center + (width / 2.0f) * value);
      fb.vline(color, x, y - aSpan, y + aSpan);
      return y;
    }

    inline float drawTarget(od::FrameBuffer &fb, od::Color color, float value) const {
      auto x = util::fhr(center + (width / 2.0f) * value);
      fb.box(color, x - 1, y - tSpan, x + 1, y + tSpan);
      return x;
    }

    const float y;

    const int hSpan;
    const int tSpan;
    const int aSpan;

    const float width;
    const float height;

    const float left;
    const float center;
    const float right;
  };
}