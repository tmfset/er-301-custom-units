local app        = app
local Class      = require "Base.Class"
local Observable = require "Base.Observable"

local Control = Class {}
Control:include(Observable)

function Control:init(args)
  self.parent   = args.parent or app.logError("%s.init: missing parent.", self)
  self.name     = args.name or app.logError("%s.init: missing name.", self)
  self.position = args.position or app.logError("%s.init: missing position.", self)

  if (args.label == nil) or args.label then
    self.button = app.SubButton(self.name, self.position)
    self:addGraphic(self.button)
  end

  self.parent:addControl(self.position, self)
end

function Control:addGraphic(graphic)
  return self.parent:addGraphic(graphic)
end

function Control:hasParentFocus(str)
  return self.parent:hasFocus(str)
end

function Control:focusParent()
  return self.parent:focus()
end

function Control:unfocusParent()
  return self.parent:unfocus()
end

function Control:onCursorEnter() end

function Control:getCursorController() end

function Control:onRemove() end
function Control:onFocus() end

function Control:onZero() end
function Control:onCancel() end
function Control:onPress(focused) end
function Control:onRelease(focused) end
function Control:onDialPress() end
function Control:onEncoder(change, shifted) end

return Control
