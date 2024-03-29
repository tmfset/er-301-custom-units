#pragma once

#include <od/graphics/FrameBuffer.h>
#include <od/graphics/Graphic.h>

#include <graphics/primitives/constants.h>
#include <graphics/primitives/Circle.h>
#include <graphics/primitives/Range.h>

#include <util/v2d.h>

namespace graphics {
  class Box {
    public:
      static inline Box lbrt(const v2d &lb, const v2d &rt) { return _lbrt(lb.min(rt), lb.max(rt)); }
      static inline Box ltrb(const v2d &lt, const v2d &rb) { return lbrt(lt.atY(rb), rb.atY(lt)); }
      static inline Box lbwh(const v2d &lb, const v2d &wh) { return lbrt(lb, lb + wh); }
      static inline Box rbwh(const v2d &rb, const v2d &wh) { return lbwh(rb.offsetX(-wh), wh); }

      static inline Box clwh(const v2d &cl, const v2d &wh) { return lbwh(cl.offsetY(-wh / 2.0f), wh); }
      static inline Box cbwh(const v2d &cb, const v2d &wh) { return lbwh(cb.offsetX(-wh / 2.0f), wh); }

      static inline Box  cwh(const v2d &c,  const v2d &wh) { return _cwh(c, wh.abs()); }
      static inline Box   cs(const v2d &c,  float s)       { return cwh(c, v2d::of(s)); }
      static inline Box   wh(const v2d &wh)                { return lbwh(v2d::zero(), wh); }

      inline Box pad(const v2d &m)            const { return lbrt(leftBottom() + m, rightTop() - m); }
      inline Box pad(float x, float y)        const { return pad(v2d::of(x, y)); }
      inline Box pad(float m)                 const { return pad(v2d::of(m)); }
      inline Box padX(float x)                const { return pad(v2d::of(x, 0)); }
      inline Box padY(float y)                const { return pad(v2d::of(0, y)); }
      inline Box padLeftBottom(const v2d &by) const { return lbrt(leftBottom() + by, rightTop()); }
      inline Box padRightTop(const v2d &by)   const { return lbrt(leftBottom(), rightTop() - by); }
      inline Box padLeft(float by)            const { return padLeftBottom(v2d::ofX(by)); }
      inline Box padBottom(float by)          const { return padLeftBottom(v2d::ofY(by)); }
      inline Box padRight(float by)           const { return padRightTop(v2d::ofX(by)); }
      inline Box padTop(float by)             const { return padRightTop(v2d::ofY(by)); }

      inline Box square(float size) const {
        return cs(center(), size);
      }

      inline Box topLeftCorner(float w, float h) const {
        return wh(widthHeight() * v2d::of(w, h)).justifyAlign(*this, LEFT_TOP);
      }

      inline Box withWidth(float w)  const { return cwh(center(), widthHeight().atX(w)); }
      inline Box withHeight(float h) const { return cwh(center(), widthHeight().atY(h)); }

      inline Box splitLeft(float by)   const { return _lbrt(leftBottom(), rightTop().atX(left() + width() * by)); }
      inline Box splitBottom(float by) const { return _lbrt(leftBottom(), rightTop().atY(bottom() + height() * by)); }
      inline Box splitRight(float by)  const { return _lbrt(leftBottom().atX(right() - width() * by), rightTop()); }
      inline Box splitTop(float by)    const { return _lbrt(leftBottom().atY(top() - height() * by), rightTop()); }

      inline Box splitLeftPad(float by, float pad) const { return splitLeft(by).padRight(pad); }
      inline Box splitRightPad(float by, float pad) const { return splitRight(by).padLeft(pad); }

      inline Box withWidthFromLeft(float width) const {
        return lbwh(leftBottom(), widthHeight().atX(width));
      }

      inline Box scale(const v2d &by)        const { return cwh(center(), widthHeight() * by); }
      inline Box scale(float w, float h)     const { return scale(v2d::of(w, h)); }
      inline Box scale(float by)             const { return scale(v2d::of(by)); }
      inline Box scaleWidth(float by)        const { return scale(v2d::of(by, 1)); }
      inline Box scaleHeight(float by)       const { return scale(v2d::of(1, by)); }
      inline Box scaleDiscrete(int w, int h) const { return scale(v2d::of(w, h)); }

      inline Box segmentQSquare(const v2d &by) const {
        return wh(widthHeight() * by.invert()).quantize().minSquare();
      }
      inline Box segmentQSquare(float w, float h) const {
        return segmentQSquare(v2d::of(w, h));
      }

      inline Box offset(const v2d &by)    const { return _lbrt(leftBottom() + by, rightTop() + by); }
      inline Box offset(float x, float y) const { return offset(v2d::of(x, y)); }
      inline Box offsetX(float x)         const { return offset(v2d::of(x, 0)); }
      inline Box offsetY(float y)         const { return offset(v2d::of(0, y)); }

      inline Box quantizeSize()   const { return cwh(center(), widthHeight().quantize()); }
      inline Box quantizeCenter() const { return cwh(center().quantize(), widthHeight()); }
      inline Box quantize()       const { return quantizeSize().quantizeCenter(); }

      inline Box recenter(const v2d &c)       const { return cwh(c, widthHeight()); }
      inline Box recenterOn(const Box& other) const { return recenter(other.center()); }
      inline Box recenterY(float y)           const { return recenter(center().atY(y)); }
      inline Box recenterX(float x)           const { return recenter(center().atX(x)); }

      inline float width()       const { return widthHeight().x(); }
      inline float height()      const { return widthHeight().y(); }
      inline v2d   widthHeight() const { return mWidthHeight; }
      inline v2d   center()      const { return mCenter; }
      inline float centerX()     const { return center().x(); }
      inline float centerY()     const { return center().y(); }

      inline float left()       const { return leftBottom().x(); }
      inline v2d   leftBottom() const { return mLeftBottom; }
      inline v2d   leftTop()    const { return mLeftBottom.atY(top()); }
      inline v2d   leftCenter() const { return center().atX(left()); }

      inline float bottom()       const { return leftBottom().y(); }
      inline v2d   bottomCenter() const { return center().atY(bottom()); }

      inline float right()       const { return rightTop().x(); }
      inline v2d   rightBottom() const { return rightTop().atY(bottom()); }
      inline v2d   rightTop()    const { return mRightTop; }
      inline v2d   rightCenter() const { return center().atX(right()); }

      inline float top()       const { return rightTop().y(); }
      inline v2d   topCenter() const { return center().atY(top()); }

      inline Range horizontal() const { return Range::lr(left(), right()); }
      inline Range vertical()   const { return Range::lr(bottom(), top()); }

      inline Box intersect(const Box& other) const {
        return lbrt(
          leftBottom().max(other.leftBottom()),
          rightTop().min(other.rightTop())
        );
      }

      inline v2d clamp(const v2d &v) const {
        return v.clamp(leftBottom(), rightTop());
      }

      inline Box insert(const Box& other) const {
        auto _lb = other.leftBottom().clamp(
          leftBottom(),
          clamp(rightTop() - other.widthHeight())
        );

        auto _rt = other.rightTop().clamp(
          clamp(_lb + other.widthHeight()),
          rightTop()
        );

        return lbrt(_lb, _rt);
      }

      inline Box atLeftBottom(const v2d &lb)    const { return _lbwh(lb, widthHeight()); }
      inline Box atLeftBottom(const Box &b)     const { return atLeftBottom(b.leftBottom());}
      inline Box atLeftBottom(float l, float b) const { return atLeftBottom(v2d::of(l, b)); }
      inline Box zeroLeftBottom()               const { return atLeftBottom(v2d::zero()); }

      inline Box atRightBottom(const v2d &rb)    const { return _rbwh(rb, widthHeight()); }
      inline Box atRightBottom(const Box &b)     const { return atRightBottom(b.rightBottom()); }
      inline Box atRightBottom(float r, float b) const { return atRightBottom(v2d::of(r, b)); }

      inline Box atLeft(float left)     const { return _lbwh(leftBottom().atX(left), widthHeight()); }
      inline Box atRight(float right)   const { return atLeft(right).offsetX(-width()); }
      inline Box atCenterX(float x)     const { return cwh(center().atX(x), widthHeight()); }
      inline Box atBottom(float bottom) const { return _lbwh(leftBottom().atY(bottom), widthHeight()); }
      inline Box atTop(float top)       const { return atBottom(top).offsetY(-height()); }
      inline Box atCenterY(float y)     const { return cwh(center().atY(y), widthHeight()); }

      inline Box outLeft(float out)   const { return withWidth(out).atRight(left()); }
      inline Box outRight(float out)  const { return withWidth(out).atLeft(right()); }
      inline Box outBottom(float out) const { return withHeight(out).atTop(bottom()); }
      inline Box outTop(float out)    const { return withHeight(out).atBottom(top()); }

      inline Box inLeft(float in)   const { return withWidth(in).atLeft(left()); }
      inline Box inRight(float in)  const { return withWidth(in).atRight(right()); }
      inline Box inBottom(float in) const { return withHeight(in).atBottom(bottom()); }
      inline Box inTop(float in)    const { return withHeight(in).atTop(top()); }

      inline Box alignLeft(const Box &within)    const { return atLeft(within.left()); }
      inline Box alignRight(const Box &within)   const { return atRight(within.right()); }
      inline Box alignCenterX(const Box &within) const { return atCenterX(within.centerX()); }
      inline Box alignBottom(const Box &within)  const { return atBottom(within.bottom()); }
      inline Box alignTop(const Box &within)     const { return atTop(within.top()); }
      inline Box alignCenterY(const Box &within) const { return atCenterY(within.centerY()); }

      inline float  minDimension() const { return widthHeight().minDimension(); }
      inline Box    minSquare()    const { return cs(center(), minDimension()); }
      inline Circle minCircle()    const { return Circle::cr(center(), minDimension() / 2.0f); }

      static inline Box extractFrom(const od::Rect &rect) {
        return _lbrt(v2d::of(rect.left, rect.bottom), v2d::of(rect.right, rect.top));
      }

      static inline Box extractWorld(od::Graphic &graphic) {
        return extractFrom(graphic.getWorldRect());
      }

      inline void applyTo(od::Graphic &graphic) const {
        graphic.setPosition(left(), bottom());
        graphic.setSize(width(), height());
      }

      inline void applyTo(od::Graphic &graphic, od::Graphic &parent) const {
        graphic.setPosition(left() - parent.mWorldLeft, bottom() - parent.mWorldBottom);
        graphic.setSize(width(), height());
      }

      inline Box justify(const Box &within, od::Justification j) const {
        switch (j) {
          case od::justifyLeft:   return alignLeft(within);
          case od::justifyCenter: return alignCenterX(within);
          case od::justifyRight:  return alignRight(within);
        }
        return alignLeft(within);
      }

      inline Box justify(float x, od::Justification j) const {
        switch (j) {
          case od::justifyLeft:   return atLeft(x);
          case od::justifyCenter: return atCenterX(x);
          case od::justifyRight:  return atRight(x);
        }
        return atLeft(x);
      }

      inline Box align(const Box &within, od::Alignment a) const {
        switch (a) {
          case od::alignBottom: return alignBottom(within);
          case od::alignMiddle: return alignCenterY(within);
          case od::alignTop:    return alignTop(within);
        }
        return alignBottom(within);
      }

      inline Box align(float y, od::Alignment a) const {
        switch (a) {
          case od::alignTop:    return atTop(y);
          case od::alignBottom: return atBottom(y);
          case od::alignMiddle: return atCenterY(y);
        }
        return atTop(y);
      }

      inline Box justifyAlign(const Box &within, JustifyAlign ja) const {
        return justify(within, ja.justify).align(within, ja.align);
      }

      inline Box justifyAlign(const v2d &v, JustifyAlign ja) const {
        return justify(v.x(), ja.justify).align(v.y(), ja.align);
      }

      inline bool containsX(float x) const {
        return x > left() && x < right();
      }

      inline void fill(od::FrameBuffer &fb, od::Color color) const {
        fb.fill(color, left(), bottom(), right(), top());
      }

      inline void clear(od::FrameBuffer &fb) const {
        fb.clear(left(), bottom(), right(), top());
      }

      inline void lineTopIn(od::FrameBuffer &fb, od::Color color, int dotting = 0) const {
        fb.hline(color, left() + 1, right() - 1, top(), dotting);
      }

      inline void lineBottomIn(od::FrameBuffer &fb, od::Color color, int dotting = 0) const {
        fb.hline(color, left() + 1, right() - 1, bottom(), dotting);
      }

      inline void traceRight(od::FrameBuffer &fb, od::Color color, int dotting = 0) const {
        fb.vline(color, right(), bottom(), top(), dotting);
      }

      inline void trace(od::FrameBuffer &fb, od::Color color) const {
        fb.box(color, left(), bottom(), right(), top());
      }

      inline void outline(od::FrameBuffer &fb, od::Color color, int by = 1) const {
        fb.box(color, left() - by, bottom() - by, right() + by, top() + by);
      }

      inline void pointAtMe(od::CursorState &state, int offset = 10) const {
        v2d xy;
        switch (state.orientation) {
          case od::cursorDown:  xy = topCenter().offset(0, offset);      break;
          case od::cursorUp:    xy = bottomCenter().offset(0, -offset);  break;
          case od::cursorLeft:  xy = rightCenter().offset(offset, -1); break;
          case od::cursorRight: xy = leftCenter().offset(-offset, -1); break;
        }

        state.x = xy.x();
        state.y = xy.y();
      }

    private:
      inline Box(const v2d &leftBottom, const v2d &rightTop) :
        mLeftBottom(leftBottom),
        mRightTop(rightTop),
        mWidthHeight(mRightTop - mLeftBottom),
        mCenter(mLeftBottom + mWidthHeight * 0.5) { }

      static inline Box _lbrt(const v2d &lb, const v2d &rt) { return Box(lb, rt); }
      static inline Box _lbwh(const v2d &lb, const v2d &wh) { return _lbrt(lb, lb + wh); }
      static inline Box _rbwh(const v2d &rb, const v2d &wh) { return _lbwh(rb.offsetX(-wh), wh); }
      static inline Box _ltrb(const v2d &lt, const v2d &rb) { return _lbrt(lt.atY(rb), rb.atY(lt)); }
      static inline Box  _cwh(const v2d &c,  const v2d &wh) { return _lbwh(c - wh / 2.0f, wh); }

      const v2d mLeftBottom;
      const v2d mRightTop;
      const v2d mWidthHeight;
      const v2d mCenter;
  };
}