local app   = app
local Class = require "Base.Class"

local ControlGroup = Class {}

function ControlGroup:init(args)
  self.setCursorController = args.setCursorController
  self.controls = {}
end

function ControlGroup:add(position, control)
  if self.controls[position] then
    app.logError("%s.register: control already exists in position %s", self, position)
    return
  end

  self.controls[position] = control
end

function ControlGroup:controlAt(position)
  return self.controls[position]
end

function ControlGroup:focusAt(position)
  local control = self:controlAt(position)
  if not control then return end
  self.focusedPosition = position
  control:onFocus()
end

function ControlGroup:cursorController()
  local control = self:focusedControl()
  if control then return control:cursorController() end
end

function ControlGroup:focusedControl()
  return self:controlAt(self.focusedPosition)
end

function ControlGroup:isFocused(position)
  return self.focusedPosition == position
end

function ControlGroup:onRemove()
  for _, control in pairs(self.controls) do
    control:onRemove()
  end
end

function ControlGroup:onPressed(position)
  local control = self:controlAt(position)
  if control then control:onPressed(self:isFocused(position)) end
  return true
end

function ControlGroup:onReleased(position)
  local control = self:controlAt(position)
  if control then control:onReleased(self:isFocused(position)) end
  return true
end

function ControlGroup:onDialPressed()
  local control = self:focusedControl()
  if control then control:onDialPressed() end
  return true
end

function ControlGroup:onDialMoved()
  local control = self:focusedControl()
  if control then control:onDialMoved() end
  return true
end

return ControlGroup
