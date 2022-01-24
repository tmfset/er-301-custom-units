local app   = app
local ply   = app.SECTION_PLY
local Class = require "Base.Class"

local View = Class {}

function View:init(args)
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

function View:setViewControl(viewControl)
  self.viewControl = viewControl
end

function View:addControl(position, control)
  if self.controls[position] then
    app.logError("%s.register: control already exists in position %s", self, position)
    return
  end

  self.controls[position] = control
end

function View:addGraphic(graphic)
  self.graphic:addChild(graphic)
end

function View:hasFocus(str)
  return self.viewControl:hasFocus(str)
end

function View:focus()
  return self.viewControl:focus()
end

function View:unfocus()
  return self.viewControl:unfocus()
end

function View:getControl(position)
  return self.controls[position]
end

function View:getFocusedControl()
  if self.position then return self:getControl(self.position) end
end

function View:clearFocusedPosition()
  self.viewControl:setSubCursorController(nil)
end

function View:setFocusedPosition(position)
  local newControl = self:getControl(position)
  if not newControl then return end

  local cursorController = newControl:getCursorController()
  if not cursorController then return end

  self.position = position
  newControl:onFocus()
  self.viewControl:setSubCursorController(cursorController)
end

function View:isPositionFocused(position)
  return self.position == position
end

function View:onRemove()
  for _, control in pairs(self.controls) do
    control:onRemove()
  end
end

function View:onFocused() end

function View:onCursorEnter()
  for _, control in pairs(self.controls) do
    control:onCursorEnter()
  end
end

function View:zeroPressed()
  local control = self:getFocusedControl()
  if control then control:onZero() end
  return true
end

function View:cancelReleased()
  local control = self:getFocusedControl()
  if control then control:onCancel() end
  return true
end

function View:spotReleased(spot, shifted) end

function View:subReleased(i)
  local control = self:getControl(i)
  if control then control:onRelease(self:isPositionFocused(i)) end
  self:setFocusedPosition(i)
  return true
end

function View:subPressed(i)
  local control = self:getControl(i)
  if control then control:onPress(self:isPositionFocused(i)) end
  return true
end

function View:dialPressed()
  local control = self:getFocusedControl()
  if control then control:onDialPress() end
  return true
end

function View:encoder(change, shifted)
  local control = self:getFocusedControl()
  if control then control:onEncoder(change, shifted) end
  return true
end

return View
