#pragma once

#include "util.h"
#include "HasChartData.h"
#include "HasScaleData.h"
#include <slew.h>
#include <vector>
#include "v2d.h"

using namespace common;

namespace graphics {
  struct Point {
    inline Point(float x, float y) : v(v2d::of(x, y)) { }
    inline Point(const v2d &_v) : v(_v) { }

    static inline Point of(const v2d &_v) {
      return Point(_v);
    }

    inline Point offset(const Point &by) const {
      return Point(v + by.v);
    }

    inline Point scale(float by) const {
      return Point(v * by);
    }

    inline Point offsetX(float byX) const {
      return Point(v + v2d::of(byX, 0));
    }

    inline Point offsetY(float byY) const {
      return Point(v + v2d::of(0, byY));
    }

    inline Point atX(float x) const {
      return Point(v.atX(x));
    }

    inline Point atY(float y) const {
      return Point(v.atY(y));
    }

    inline Point quantize() const {
      return Point(v.quantize());
    }

    inline void diamond(od::FrameBuffer &fb, od::Color color) const {
      fb.pixel(color, v.x() - 1, v.y());
      fb.pixel(color, v.x() + 1, v.y());
      fb.pixel(color, v.x(), v.y() - 1);
      fb.pixel(color, v.x(), v.y() + 1);
    }

    inline void dot(od::FrameBuffer &fb, od::Color color) const {
      fb.pixel(color, v.x(), v.y());
    }

    const v2d v;
  };

  class Line {
    public:
      inline Line(const v2d &f, const v2d &t) :
        from(f), to(t) { }

      static inline Line of(const v2d &f, const v2d &t) { return Line(f, t); }

      inline Line retreat(const v2d &f) const { return of(f, from); }
      inline Line advance(const v2d &t) const { return of(to, t); }

      inline void trace(od::FrameBuffer &fb, od::Color color) const {
        fb.line(color, from.x(), from.y(), to.x(), to.y());
      }

    private:
      v2d from;
      v2d to;
  };

  struct Circle {
    inline Circle(const v2d &c, float r) :
      center(c),
      radius(r) { }

    static inline Circle cr(const v2d &c, float r) {
      return Circle(c, r);
    }

    inline Circle offset(const v2d &by) const {
      return cr(center + by, radius);
    }

    inline Circle scale(float by) const {
      return cr(center, radius * by);
    }

    inline Circle grow(float by) const {
      return cr(center, radius + by);
    }

    inline v2d pointAtTheta(float theta) const {
      return center + v2d::ar(theta, radius);
    }

    inline void trace(od::FrameBuffer &fb, od::Color color) const {
      fb.circle(color, center.x(), center.y(), radius);
    }

    inline void fill(od::FrameBuffer &fb, od::Color color) const {
      fb.fillCircle(color, center.x(), center.y(), radius);
    }

    const v2d center;
    const float radius;
  };

  struct Box {
    inline Box(const v2d &_lb, const v2d &_rt) :
        leftBottom(_lb),
        rightTop(_rt),
        widthHeight(_rt - _lb),
        center(leftBottom + widthHeight * 0.5) { }

    static inline Box lbrt_raw(const v2d &_lb, const v2d &_rt) {
      return Box(_lb, _rt);
    }

    static inline Box lbrt(const v2d &_lb, const v2d &_rt) {
      return lbrt_raw(_lb.min(_rt), _lb.max(_rt));
    }

    static inline Box lbwh_raw(const v2d &_lb, const v2d &_wh) {
      return lbrt_raw(_lb, _lb + _wh);
    }

    static inline Box ltrb(const v2d &lt, const v2d &rb) {
      return lbrt(v2d::of(lt.x(), rb.y()), v2d::of(rb.x(), lt.y()));
    }

    static inline Box ltrb_raw(const v2d &lt, const v2d &rb) {
      return lbrt_raw(v2d::of(lt.x(), rb.y()), v2d::of(rb.x(), lt.y()));
    }

    static inline Box lbwh(const v2d &_lb, const v2d &_wh) {
      return lbrt(_lb, _lb + _wh);
    }

    static inline Box cwr(const v2d &c, const v2d &wr) {
      auto hw = wr.x() / 2.0f;
      return lbrt(
        v2d::of(c.x() - hw, c.y()),
        v2d::of(c.x() + hw, c.y() + wr.y())
      );
    }

    static inline Box cwh(const v2d &c, const v2d &_wh) {
      auto wha = _wh.abs();
      return lbwh_raw(c - wha / 2.0f, wha);
    }

    static inline Box cs(const v2d &c, float s) {
      return cwh(c, v2d::of(s));
    }

    inline Box inner(const v2d &m) const {
      return lbrt(leftBottom + m, rightTop - m);
    }

    inline Box inner(float x, float y) const {
      return inner(v2d::of(x, y));
    }

    inline Box inner(float m)  const { return inner(v2d::of(m)); }
    inline Box innerX(float x) const { return inner(v2d::of(x, 0)); }
    inline Box innerY(float y) const { return inner(v2d::of(0, y)); }

    inline Box square(float size) const {
      return cs(center, size);
    }

    inline Box padRight(float by) const {
      return lbrt(leftBottom, rightTop - v2d::of(by, 0));
    }

    inline Box topLeftCorner(float w, float h) const {
      auto _wh = widthHeight * v2d::of(w, h);
      auto _lb = leftBottom;
      auto _rt = rightTop;

      return lbrt_raw(
        v2d::of(_lb.x(), _rt.y() - _wh.y()),
        v2d::of(_lb.x() + _wh.x(), _rt.y())
      );
    }

    inline Box divideTop(float by) const {
      auto _wh = widthHeight;
      auto _lb = leftBottom;
      auto _rt = rightTop;
      return lbrt_raw(v2d::of(_lb.x(), _rt.y() - _wh.y() * by), _rt);
    }

    inline Box divideBottom(float by) const {
      auto _wh = widthHeight;
      auto _lb = leftBottom;
      auto _rt = rightTop;
      return lbrt_raw(_lb, v2d::of(_rt.x(), _lb.y() + _wh.y() * by));
    }

    inline Box scale(const v2d &by) const {
      return cwh(center, widthHeight * by);
    }

    inline Box scale(float by) const {
      return scale(v2d::of(by));
    }

    inline Box scaleWidth(float by) const {
      return scale(v2d::of(by, 1));
    }

    inline Box scaleHeight(float by) const {
      return scale(v2d::of(1, by));
    }

    inline Box withWidth(float w) const {
      return cwh(center, widthHeight.atX(w));
    }

    inline Box withHeight(float h) const {
      return cwh(center, widthHeight.atY(h));
    }

    inline Box splitLeft(float by) const {
      auto _wh = widthHeight;
      auto _lb = leftBottom;
      auto _rt = rightTop;
      return lbrt_raw(_lb, v2d::of(_lb.x() + _wh.y() * by, _rt.y()));
    }

    inline Box splitRight(float by) const {
      auto _wh = widthHeight;
      auto _lb = leftBottom;
      auto _rt = rightTop;
      return lbrt_raw(v2d::of(_rt.x() - _wh.x() * by, _lb.y()), _rt);
    }

    inline Box withWidthFromLeft(float _width) const {
      return lbwh(leftBottom, widthHeight.atX(_width));
    }

    inline Box offset(const v2d &by) const {
      return lbrt_raw(leftBottom + by, rightTop + by);
    }

    inline Box offset(float x, float y) const {
      return offset(v2d::of(x, y));
    }

    inline Box offsetX(float x) const {
      return offset(v2d::of(x, 0));
    }

    inline Box offsetY(float y) const {
      return offset(v2d::of(0, y));
    }

    inline Box quantizeSize() const {
      return cwh(center, widthHeight.quantize());
    }

    inline Box quantizeCenter() const {
      return cwh(center.quantize(), widthHeight);
    }

    inline Box quantize() const {
      return quantizeSize().quantizeCenter();
    }

    inline Box intersect(const Box& other) const {
      return lbrt(
        leftBottom.max(other.leftBottom),
        rightTop.min(other.rightTop)
      );
    }

    inline v2d rightCenter() const {
      return center.atX(rightTop.x());
    }

    inline Box recenter(const v2d &c) const {
      return cwh(c, widthHeight);
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

    inline v2d clamp(const v2d &v) const {
      return v.clamp(leftBottom, rightTop);
    }

    inline Box insert(const Box& other) const {
      auto _lb = other.leftBottom.clamp(
        leftBottom,
        clamp(rightTop - other.widthHeight)
      );

      auto _rt = other.rightTop.clamp(
        clamp(_lb + other.widthHeight),
        rightTop
      );

      return lbrt(_lb, _rt);
    }

    inline Box alignLeftBottom(const Box &other) const {
      return lbwh_raw(other.leftBottom, widthHeight);
    }

    inline Box alignRightBottom(const Box &other) const {
      auto _wh = widthHeight;
      auto _lb = other.leftBottom;
      auto _rt = other.rightTop;
      return lbrt_raw(
        v2d::of(_rt.x() - _wh.x(), _lb.y()),
        v2d::of(_rt.x(), _lb.y() + _wh.y())
      );
    }

    inline float minDimension() const {
      return widthHeight.minDimension();
    }

    inline Box minSquare() const {
      return cs(center, minDimension());
    }

    inline Box scaleDiscrete(int w, int h) const {
      return scale(v2d::of(w, h));
    }

    inline bool containsX(float x) const {
      return x > leftBottom.x() && x < rightTop.x();
    }

    inline void fill(od::FrameBuffer &fb, od::Color color) const {
      fb.fill(color, leftBottom.x(), leftBottom.y(), rightTop.x(), rightTop.y());
    }

    inline void clear(od::FrameBuffer &fb) const {
      fb.clear(leftBottom.x(), leftBottom.y(), rightTop.x(), rightTop.y());
    }

    inline void lineTopIn(od::FrameBuffer &fb, od::Color color, int dotting = 0) const {
      fb.hline(color, leftBottom.x() + 1, rightTop.x() - 1, rightTop.y(), dotting);
    }

    inline void lineBottomIn(od::FrameBuffer &fb, od::Color color, int dotting = 0) const {
      fb.hline(color, leftBottom.x() + 1, rightTop.x() - 1, leftBottom.y(), dotting);
    }

    inline void trace(od::FrameBuffer &fb, od::Color color) const {
      fb.box(color, leftBottom.x(), leftBottom.y(), rightTop.x(), rightTop.y());
    }

    inline void outline(od::FrameBuffer &fb, od::Color color) const {
      fb.box(color, leftBottom.x() - 1, leftBottom.y() - 1, rightTop.x() + 1, rightTop.y() + 1);
    }

    inline float width()  const { return widthHeight.x(); }
    inline float height() const { return widthHeight.y(); }

    inline float left()   const { return leftBottom.x(); }
    inline float bottom() const { return leftBottom.y(); }
    inline float right()  const { return rightTop.x(); }
    inline float top()    const { return rightTop.y(); }

    inline Circle minCircle() const {
      return Circle::cr(center, minDimension() / 2.0f);
    }

    const v2d leftBottom;
    const v2d rightTop;
    const v2d widthHeight;
    const v2d center;
  };

  struct Grid {
    inline Grid(const Box &topLeft, int _cols, int _rows, float pad) :
      mark(topLeft.quantize().inner(pad)),
      cols(_cols),
      rows(_rows),
      cStep(topLeft.widthHeight.x()),
      rStep(-topLeft.widthHeight.y()) { }

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

    static inline IFader cwh(const v2d &c, const v2d &wh) {
      auto wh2 = wh * 0.5;
      return IFader(c.x(), c.y() - wh2.y(), c.y() + wh2.y(), wh2.x());
    }

    static inline IFader box(const Box &b) {
      return IFader(b.center.x(), b.bottom(), b.top(), b.width());
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

    static inline HFader cwh(const v2d &c, const v2d &wh) {
      auto wh2 = wh * 0.5;
      return HFader(c.y(), c.x() - wh2.x(), c.x() + wh2.x(), wh2.y());
    }

    static inline HFader box(const Box &b) {
      return HFader(b.center.y(), b.left(), b.right(), b.height() / 2.0f);
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

  struct Keyboard {
    //  1 3   6 8 A
    // 0 2 4 5 7 9 B
    static inline bool isBlackKey(float key) {
      int i = util::fhr(key);
      if (i == 1) return true;
      if (i == 3) return true;
      if (i == 6) return true;
      if (i == 8) return true;
      if (i == 10) return true;
      return false;
    }

    static inline v2d keyPosition(const v2d &runRise, float key) {
      float offset = key > 4 ? 1 : 0;
      return runRise * v2d::of(key + offset, isBlackKey(key) ? 1 : 0);
    }

    static inline void draw(
      od::FrameBuffer &fb,
      common::HasScaleData &data,
      const Circle &base,
      const v2d &runRise
    ) {
      auto detected = data.getDetectedCentValue() / 100.0f;
      auto quantized = data.getQuantizedCentValue() / 100.0f;
      base.offset(keyPosition(runRise, detected)).fill(fb, GRAY3);
      base.offset(keyPosition(runRise, quantized)).fill(fb, GRAY10);

      for (int i = 0; i < 12; i++) {
        base.offset(keyPosition(runRise, i)).trace(fb, GRAY5);
      }

      int length = data.getScaleSize();
      for (int i = 0; i < length; i++) {
        float key = data.getScaleCentValue(i) / 100.0f;
        base.offset(keyPosition(runRise, key)).trace(fb, GRAY13);
      }
    }
  };

  struct HKeyboard {
    inline HKeyboard(common::HasScaleData &data, float pad) :
        mScaleData(data),
        mPad(pad) {
      mScaleData.attach();
    }

    inline ~HKeyboard() {
      mScaleData.release();
    }

    inline void draw(od::FrameBuffer &fb, const Box &world) const {
      auto key = world.scale(v2d::of(1.0f / 7.0f, 1.0f / 2.0f)).minSquare();
      auto bounds = key.scale(v2d::of(7.0f, 2.0f));

      auto aligned  = key.alignLeftBottom(bounds);
      auto diameter = aligned.minDimension();
      auto radius   = diameter / 2.0f;

      Keyboard::draw(
        fb,
        mScaleData,
        aligned.inner(mPad).minCircle(),
        v2d::of(radius, diameter)
      );
    }

    common::HasScaleData &mScaleData;
    const float mPad;
  };

  struct IKeyboard {
    inline IKeyboard(common::HasScaleData &data, float pad) :
        mScaleData(data),
        mPad(pad) {
      mScaleData.attach();
    }

    inline ~IKeyboard() {
      mScaleData.release();
    }
    
    inline void draw(od::FrameBuffer &fb, const Box &world) const {
      auto key = world.scale(v2d::of(1.0f / 2.0f, 1.0f / 7.0f)).minSquare();
      auto bounds = key.scale(v2d::of(2.0f, 7.0f));

      auto aligned  = key.alignRightBottom(bounds);
      auto diameter = aligned.minDimension();
      auto radius   = diameter / 2.0f;

      Keyboard::draw(
        fb,
        mScaleData,
        aligned.inner(mPad).minCircle(),
        v2d::of(radius, -diameter)
      );
    }

    common::HasScaleData &mScaleData;
    float mPad;
  };

  class HChart {
    public:
      inline HChart(common::HasChartData &data, int barWidth, int barSpace) :
          mChartData(data),
          mBarWidth(barWidth),
          mBarSpace(barSpace) {
        mChartData.attach();
      }

      inline ~HChart() {
        mChartData.release();
      }

      inline void draw(od::FrameBuffer &fb, const Box& world) const {
        auto length  = mChartData.getChartSize();
        auto current = mChartData.getChartCurrentIndex();

        auto fullWidth = length * mBarWidth + (length - 1) * mBarSpace;
        auto chart     = Box::cwh(world.center, v2d::of(fullWidth, world.height()));

        auto window  = chart.insert(world.recenterX(chart.left() + barCenter(current)));
        auto view    = window.recenterOn(world);

        for (int i = 0; i < length; i++) {
          auto cx = chart.left() + barCenter(i);
          auto xy = view.center.atX(cx - window.left() + view.left());
          if (!view.containsX(xy.x())) continue;

          auto h  = mChartData.getChartValue(i) * world.height() / 2.0f;
          auto wh = v2d::of(mBarWidth, h);

          auto isCurrent = i == current;
          Box::cwr(xy, wh).fill(fb, isCurrent ? GRAY12 : GRAY10);
          if (!isCurrent) continue;

          Box::cs(xy, mBarWidth + 2).trace(fb, WHITE);
        }
      }

    private:
      inline float barStart(int i) const {
        return mBarWidth * i + mBarSpace * i;
      }

      inline float barCenter(int i) const {
        return barStart(i) + mBarWidth / 2.0f;
      }

      common::HasChartData &mChartData;

      int mBarWidth;
      int mBarSpace;
  };

  class CircleChart {
    public:
      inline CircleChart(common::HasChartData &data, float size) :
          mChartData(data),
          mSize(size) {
        mChartData.attach();
      }

      inline ~CircleChart() {
        mChartData.release();
      }

      inline void draw(od::FrameBuffer &fb, const Box& world) const {
        auto length  = mChartData.getChartSize();
        auto current = mChartData.getChartCurrentIndex();

        auto outer  = world.minCircle();
        auto inner  = outer.scale(mSize);
        auto span   = (outer.radius - inner.radius) / 2.0f;
        auto center = inner.grow(span);

        inner.trace(fb, GRAY1);
        center.trace(fb, GRAY5);
        outer.trace(fb, GRAY1);

        if (length < 1) return;

        float delta = M_PI * 2.0f / (float)length;

        v2d first, prev, curr;
        for (int i = 0; i < length; i++) {
          auto next = getCirclePoint(center, span, delta, i);

          if (i == 0) first = next;
          if (i == current) curr = next;

          if (i > 0) Line::of(prev, next).trace(fb, GRAY10);
          
          prev = next;
        }

        Line::of(prev, first).trace(fb, GRAY10);

        Point::of(curr).diamond(fb, WHITE);
      }

    private:
      v2d getCirclePoint(const Circle &center, float span, float delta, int i) const {
        float amount = mChartData.getChartValue(i);
        return center.grow(span * amount).pointAtTheta(delta * (float)i);
      }

      common::HasChartData &mChartData;
      float mSize;
  };
}