local app   = app
local Class = require "Base.Class"
local Base  = require "Unit.ViewControl.EncoderControl"

local Split = Class {}
Split:include(Base)

function Split:init(args)
  Base.init(self, args.name)
end

function Split:addSubView(subView)
  if self._subview then
    app.logError("%s.addSubView: subView already set.", self)
    return
  end

  self._subView = subView
  self._subView:setViewControl(self)
  self.subGraphic = subView.graphic
end

function Split:subView()
  if not self._subView then
    app.logError("%s.subView: missing sub view", self)
  end

  return self._subView
end

function Split:onFocused()
  self:subView():onFocused()
  Base.onFocused(self)
end

function Split:onCursorEnter()
  self:subView():onCursorEnter()
  Base.onCursorEnter(self)
end

function Split:zeroPressed()
  return self:subView():zeroPressed()
end

function Split:cancelReleased()
  return self:subView():cancelReleased()
end

function Split:subReleased(i, shifted)
  if shifted then return false end
  return self:subView():subReleased(i, shifted)
end

function Split:subPressed(i, shifted)
  if shifted then return false end
  return self:subView():subPressed(i)
end

function Split:dialPressed(shifted)
  return self:subView():dialPressed(shifted)
end

function Split:encoder(change, shifted)
  return self:subView():encoder(change, shifted)
end

function Split:unfocusSubView()
  self:subView():clearFocusedPosition()
  self:unfocus()
end

return Split
