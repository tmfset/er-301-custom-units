local app = app
local Class = require "Base.Class"
local Encoder = require "Encoder"
local ply = app.SECTION_PLY

local SubView = Class {}

function SubView:setViewControl(viewControl)
  self.viewControl = viewControl
end

function SubView:hasFocus(str)
  self.viewControl:hasFocus(str)
end

function SubView:focus()
  self.viewControl:focus()
end

function SubView:unfocus()
  self.viewControl:unfocus()
end

function SubView:setSubCursorController(readout)
  self.viewControl:setSubCursorController(readout)
end

function SubView:setFocusedReadout(readout)
  if not self:hasFocus("encoder") then self:focus() end

  local wasAlreadyFocused = self.focusedReadout == readout

  if readout then readout.readout:save() end
  self.focusedReadout = readout
  self:setSubCursorController(readout.readout)

  if readout and wasAlreadyFocused then
    self.doKeyboardSet(readout)
  end
end

function SubView:zeroPressed()
  if self.focusedReadout then self.focusedReadout.readout:zero() end
  return true
end

function SubView:cancelReleased()
  if self.focusedReadout then self.focusedReadout.readout:restore() end
  return true
end

function SubView.doKeyboardSet(args)
  local Decimal = require "Keyboard.Decimal"

  local keyboard = Decimal {
    message       = args.message,
    commitMessage = args.commitMessage,
    initialValue  = args.readout:getValueInUnits()
  }

  local task = function(value)
    if value then
      args.readout:save()
      args.readout:setValueInUnits(value)
      self:unfocus()
    end
  end

  keyboard:subscribe("done", task)
  keyboard:subscribe("commit", task)
  keyboard:show()
end

function SubView:getReadoutByIndex(i)
  return nil
end

return SubView