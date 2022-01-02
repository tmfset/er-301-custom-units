local app     = app
local ply     = app.SECTION_PLY

local Class   = require "Base.Class"
local Encoder = require "Encoder"
local Base    = require "common.assets.ViewControl.Sub.Control"

local Readout = Class {}
Readout:include(Base)

function Readout:init(args)
  Base.init(self, args)

  local param = args.parameter or app.logError("%s.addReadout: missing parameter.", self)
  param:enableSerialization()

  local units      = args.units or app.unitNone
  local encoderMap = args.encoderMap or Encoder.getMap("default")
  local precision  = args.precision or 2
  local column     = args.column or app.BUTTON1_CENTER
  local row        = args.row or app.GRID5_CENTER1

  self.readout = app.Readout(0, 0, ply, 10)
  self.readout:setParameter(param)
  self.readout:setAttributes(units, encoderMap)
  self.readout:setPrecision(precision)
  self.readout:setCenter(column, row)
  self:addGraphic(self.readout)

  self.editMessage   = args.editMessage or ""
  self.commitMessage = args.commitMessage or ""
  self.encoderState  = args.encoderState or Encoder.Coarse
end

function Readout:doKeyboardSet()
  local Decimal = require "Keyboard.Decimal"

  local keyboard = Decimal {
    message       = self.editMessage,
    commitMessage = self.commitMessage,
    initialValue  = self.readout:getValueInUnits()
  }

  local task = function(value)
    if value then
      self.readout:save()
      self.readout:setValueInUnits(value)
    end
  end

  keyboard:subscribe("done", task)
  keyboard:subscribe("commit", task)
  keyboard:show()
end

function Readout:onFocus()
  if not self:hasParentFocus("encoder") then self:focusParent() end
  self.readout:save()
end

function Readout:getCursorController()
  return self.readout
end

function Readout:onZero()
  self.readout:zero()
end

function Readout:onCancel(focused)
  self.readout:restore()
end

function Readout:onRelease(focused)
  if focused then self:doKeyboardSet() end
end

function Readout:onEncoder(change, shifted)
  self.readout:encoder(change, shifted, self.encoderState == Encoder.Fine)
end

return Readout
