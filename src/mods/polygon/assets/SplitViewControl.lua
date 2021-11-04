local app = app
local Class = require "Base.Class"
local Base = require "Unit.ViewControl.EncoderControl"

local SplitViewControl = Class {}
SplitViewControl:include(Base)

function SplitViewControl:init(args)
  Base.init(self, args.name)
end

function SplitViewControl:addSubView(subView)
  if self._subview then
    app.logError("%s.addSubView: subView already set.", self)
    return
  end

  self._subView = subView
  self._subView:setViewControl(self)
  self.subGraphic = subView.graphic
end

function SplitViewControl:subView()
  if not self._subView then
    app.logError("%s.subView: missing sub view", self)
  end

  return self._subView
end

function SplitViewControl:onFocused()
  self:subView():onFocused()
  Base.onFocused(self)
end

function SplitViewControl:onCursorEnter()
  self:subView():onCursorEnter()
  Base.onCursorEnter(self)
end

function SplitViewControl:zeroPressed()
  return self:subView():zeroPressed()
end

function SplitViewControl:cancelReleased()
  return self:subView():cancelReleased()
end

function SplitViewControl:subReleased(i, shifted)
  if shifted then return false end
  return self:subView():subReleased(i, shifted)
end

function SplitViewControl:subPressed(i, shifted)
  if shifted then return false end
  return self:subView():subPressed(i)
end

function SplitViewControl:encoder(change, shifted)
  return self:subView():encoder(change, shifted)
end

function SplitViewControl:unfocusSubView()
  self:subView():clearFocusedPosition()
  self:unfocus()
end

return SplitViewControl