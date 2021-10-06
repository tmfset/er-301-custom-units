local app = app
local Class = require "Base.Class"
local Observable = require "Base.Observable"

local SubControl = Class {}
SubControl:include(Observable)

function SubControl:init(args)
  self.parent   = args.parent or app.logError("%s.init: missing parent.", self)
  self.name     = args.name or app.logError("%s.init: missing name.", self)
  self.position = args.position or app.logError("%s.init: missing position.", self)
  self.button   = app.SubButton(self.name, self.position)
  self:addGraphic(self.button)

  self.parent:addControl(self.position, self)
end

function SubControl:addGraphic(graphic)
  return self.parent:addGraphic(graphic)
end

function SubControl:hasParentFocus(str)
  return self.parent:hasFocus(str)
end

function SubControl:focusParent()
  return self.parent:focus()
end

function SubControl:unfocusParent()
  return self.parent:unfocus()
end

function SubControl:onCursorEnter() end

function SubControl:getCursorController() end

function SubControl:onRemove() end
function SubControl:onFocus() end

function SubControl:onZero() end
function SubControl:onCancel() end
function SubControl:onPress(focused) end
function SubControl:onRelease(focused) end
function SubControl:onEncoder(change, shifted) end

return SubControl