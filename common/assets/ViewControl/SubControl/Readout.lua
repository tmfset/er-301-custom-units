local app     = app
local ply     = app.SECTION_PLY

local common  = require "common.lib"
local DialMap = require "common.assets.DialMap"
local Class   = require "Base.Class"
local Encoder = require "Encoder"
local Base    = require "common.assets.ViewControl.SubControl"

local Readout = Class {}
Readout:include(Base)

function Readout:init(args)
  Base.init(self, args)

  local param = args.parameter or app.logError("%s.addReadout: missing parameter.", self)
  param:enableSerialization()

  local units     = args.units     or common.unitNone
  local dialMap   = args.dialMap   or DialMap.punit.default
  local precision = args.precision or 2
  local column    = args.column    or app.BUTTON1_CENTER
  local row       = args.row       or app.GRID5_CENTER1

  self.readout = common.ParameterReadout(param)
  self.readout:setAttributes(units, dialMap)
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
  Encoder.set(self.encoderState)
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

function Readout:onDialPress()
  if self.encoderState == Encoder.Coarse then
    self.encoderState = Encoder.Fine
  else
    self.encoderState = Encoder.Coarse
  end
  Encoder.set(self.encoderState)
end

function Readout:onEncoder(change, shifted)
  self.readout:encoder(change, shifted, self.encoderState == Encoder.Fine)
end

return Readout
