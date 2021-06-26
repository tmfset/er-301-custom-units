local app = app
local Class = require "Base.Class"
local Base = require "Unit.ViewControl.EncoderControl"
local Encoder = require "Encoder"
local ply = app.SECTION_PLY

local SidechainMeter = Class {
  type = "Meter",
  canEdit = false,
  canMove = false
}
SidechainMeter:include(Base)

function SidechainMeter:init(args)
  Base.init(self)
  self:setClassName("strike.SidechainMeter")
  local button = args.button or app.logError("%s.init: button is missing.", self)
  self:setInstanceName(button)
  local branch = args.branch or app.logError("%s.init: branch is missing.", self)
  local compressor = args.compressor or app.logError("%s.init: compressor is missing.", self)
  local channelCount = args.channelCount or app.logError("%s.init: channelCount is missing.", self)

  local defaults = {
    map = args.map or app.logError("%s.init: map is missing.", self),
    units = args.units or app.logError("%s.init: units is missing.", self),
    scaling = args.scaling or app.logError("%s.init: scaling is missing.", self)
  }

  self.defaults = defaults

  self.branch = branch
  self.compressor = compressor
  self.channelCount = channelCount

  self.fader = (function ()
    local fader = app.Fader(0, 0, ply, 64)
    fader:setParameter(compressor:getParameter("Input Gain"))
    fader:setLabel(button)
    fader:setAttributes(defaults.units, defaults.map, defaults.scaling)
    fader:setTextBelow(-59, "-inf dB")
    fader:setPrecision(1)
    return fader
  end)()

  self:setMainCursorController(self.fader)
  self:setControlGraphic(self.fader)
  self:addSpotDescriptor {
    center = 0.5 * ply
  }

  -- sub display
  graphic = app.Graphic(0, 0, 128, 64)
  self.subGraphic = graphic

  self.scope = app.MiniScope(0, 15, ply, 64 - 15)
  self.scope:setBorder(1)
  self.scope:setCornerRadius(3, 3, 3, 3)
  graphic:addChild(self.scope)

  self.subButton1 = app.SubButton("sidechain", 1)
  graphic:addChild(self.subButton1)

  self.enableButton = app.TextPanel("Enable", 2, 0.25)
  self.enableIndicator = app.BinaryIndicator(0, 24, ply, 32)
  self.enableButton:setLeftBorder(0)
  self.enableButton:addChild(self.enableIndicator)
  graphic:addChild(self.enableButton)

  branch:subscribe("contentChanged", self)
end

function SidechainMeter:onRemove()
  self.branch:unsubscribe("contentChanged", self)
  Base.onRemove(self)
end

function SidechainMeter:getPinControl()
  local Fader = require "PinView.Fader"

  local control = Fader {
    delegate = self,
    name = self.fader:getLabel(),
    valueParam = self.fader:getValueParameter(),
    range = self.fader:getRangeObject(),
    units = self.defaults.units,
    map = self.defaults.map,
    scaling = self.defaults.scaling,
    precision = 1,
    leftOutlet = self:getLeftWatch(),
    rightOutlet = self:getRightWatch()
  }

  return control
end

function SidechainMeter:createPinMark()
  local offset
  if self.branch.channelCount < 2 then
    offset = -3
  else
    offset = 0
  end
  local Drawings = require "Drawings"
  local graphic = app.Drawing(offset, 0, app.SECTION_PLY, 64)
  graphic:add(Drawings.Control.Pin)
  self.controlGraphic:addChildOnce(graphic)
  self.pinMark = graphic
end

function SidechainMeter:isSidechainEnabled()
  return self.compressor:isSidechainEnabled()
end

function SidechainMeter:isMuted()
  return self.muteIndicator:value()
end

function SidechainMeter:isSolo()
  return self.soloIndicator:value()
end

function SidechainMeter:getLeftWatch()
  return self.compressor:getOutput("Left In Post Gain")
end

function SidechainMeter:getRightWatch()
  if self.channelCount < 2  or self.compressor:isSidechainEnabled() then
    return nil
  end

  return self.compressor:getOutput("Right In Post Gain")
end

function SidechainMeter:updateViewState()
  if self.compressor:isSidechainEnabled() then
    self.enableIndicator:on()
  else
    self.enableIndicator:off()
  end

  self.fader:watchOutlets(self:getLeftWatch(), self:getRightWatch())
end

function SidechainMeter:onInsert()
  self:updateViewState()
end

function SidechainMeter:contentChanged(chain)
  if chain == self.branch then
    self.scope:watchOutlet(chain:getMonitoringOutput(1))
  end
end

function SidechainMeter:selectReleased(i, shifted)
  self:updateViewState()
  return true
end

function SidechainMeter:zeroPressed()
  self.fader:zero()
  return true
end

function SidechainMeter:cancelReleased(shifted)
  if not shifted then self.fader:restore() end
  return true
end

function SidechainMeter:onFocused()
  self.fader:save()
end

function SidechainMeter:subReleased(i, shifted)
  if shifted then return false end

  if i == 1 then
    self:unfocus()
    self.branch:show()
  elseif i == 2 then
    self.compressor:toggleSidechainEnabled()
    self:updateViewState()
  end

  return true
end

function SidechainMeter:encoder(change, shifted)
  self.fader:encoder(change, shifted, self.encoderState == Encoder.Fine)
  return true
end

return SidechainMeter
