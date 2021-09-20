local app = app
local Class = require "Base.Class"
local ply = app.SECTION_PLY

local SubView = Class {}

function SubView:init(args)
  self.name = args.name or app.logError("%s.init: missing name.", self)
  self.graphic = app.Graphic(0, 0, 128, 64)

  local description = app.Label(self.name, 10)
  description:fitToText(3)
  description:setSize(ply * 2 - 4, description.mHeight)
  description:setBorder(1)
  description:setCornerRadius(3, 3, 3, 3)
  description:setCenter(0.5 * (app.BUTTON2_CENTER + app.BUTTON3_CENTER), app.GRID5_CENTER1)

  self:addGraphic(description)

  self.controls = {}
end

function SubView:setViewControl(viewControl)
  self.viewControl = viewControl
end

function SubView:addControl(position, control)
  if self.controls[position] then
    app.logError("%s.register: control already exists in position %s", self, position)
    return
  end

  self.controls[position] = control
end

function SubView:addGraphic(graphic)
  self.graphic:addChild(graphic)
end

function SubView:hasFocus(str)
  return self.viewControl:hasFocus(str)
end

function SubView:focus()
  return self.viewControl:focus()
end

function SubView:unfocus()
  return self.viewControl:unfocus()
end

function SubView:getControl(position)
  return self.controls[position]
end

function SubView:getFocusedControl()
  if self.position then return self:getControl(self.position) end
end

function SubView:setFocusedPosition(position)
  self.position = position

  local control = self:getFocusedControl()
  local cursorController = nil
  if control then
    control:onFocus()
    cursorController = control:getCursorController()
  end
  self.viewControl:setSubCursorController(cursorController)
end

function SubView:isPositionFocused(position)
  return self.position == position
end

function SubView:onRemove()
  for _, control in pairs(self.controls) do
    control:onRemove()
  end
end

function SubView:onCursorEnter()
  for _, control in pairs(self.controls) do
    control:onCursorEnter()
  end
end

function SubView:zeroPressed()
  local control = self:getFocusedControl()
  if control then control:onZero() end
  return true
end

function SubView:cancelReleased()
  local control = self:getFocusedControl()
  if control then control:onCancel() end
  return true
end

function SubView:subReleased(i)
  local control = self:getControl(i)
  if control then control:onRelease(self:isPositionFocused(i)) end
  self:setFocusedPosition(i)
  return true
end

function SubView:subPressed(i)
  local control = self:getControl(i)
  if control then control:onPress(self:isPositionFocused(i)) end
  return true
end

function SubView:encoder(change, shifted)
  local control = self:getFocusedControl()
  if control then control:onEncoder(change, shifted) end
  return true
end

return SubView