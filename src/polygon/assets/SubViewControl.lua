local app = app
local Class = require "Base.Class"
local Observable = require "Base.Observable"

local SubViewControl = Class {}
SubViewControl:include(Observable)

function SubViewControl:init(args)
  self.parent   = args.parent or app.logError("%s.init: missing parent.", self)
  self.name     = args.name or app.logError("%s.init: missing name.", self)
  self.position = args.position or app.logError("%s.init: missing position.", self)
  self.button   = app.SubButton(self.name, self.position)
  self:addGraphic(self.button)

  self.parent:addControl(self.position, self)
end

function SubViewControl:addGraphic(graphic)
  return self.parent:addGraphic(graphic)
end

function SubViewControl:hasParentFocus(str)
  return self.parent:hasFocus(str)
end

function SubViewControl:focusParent()
  return self.parent:focus()
end

function SubViewControl:unfocusParent()
  return self.parent:unfocus()
end

function SubViewControl:onCursorEnter() end

function SubViewControl:getCursorController() end

function SubViewControl:onRemove() end
function SubViewControl:onFocus() end

function SubViewControl:onZero() end
function SubViewControl:onCancel() end
function SubViewControl:onPress(focused) end
function SubViewControl:onRelease(focused) end
function SubViewControl:onEncoder(change, shifted) end

return SubViewControl