local app = app
local Class = require "Base.Class"
local Base = require "Unit.ViewControl.EncoderControl"
local Encoder = require "Encoder"

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
  local faderParam = args.faderParam or
                         app.logError("%s.init: faderParam is missing.", self)
  -- optional arguments
  self:setDefaults(args)
  local map = args.map
  local units = args.units
  local scaling = args.scaling

  faderParam:enableSerialization()

  self.branch = branch
  local graphic
  local ply = app.SECTION_PLY

  graphic = app.Fader(0, 0, ply, 64)
  graphic:setParameter(faderParam)
  graphic:setLabel(button)
  graphic:setAttributes(units, map, scaling)
  if units == app.unitDecibels then
    graphic:setTextBelow(-59, "-inf dB")
    graphic:setPrecision(1)
  end
  self.fader = graphic
  self:setMainCursorController(graphic)
  self:setControlGraphic(graphic)
  self:addSpotDescriptor{
    center = 0.5 * ply
  }

  -- sub display
  graphic = app.Graphic(0, 0, 128, 64)
  self.subGraphic = graphic

  self.scope = app.MiniScope(0, 15, ply, 64 - 15)
  self.scope:setBorder(1)
  self.scope:setCornerRadius(3, 3, 3, 3)
  graphic:addChild(self.scope)

  self.subButton1 = app.SubButton("empty", 1)
  graphic:addChild(self.subButton1)

  self.subButton2 = app.TextPanel("Solo", 2, 0.25)
  self.soloIndicator = app.BinaryIndicator(0, 24, ply, 32)
  self.subButton2:setLeftBorder(0)
  self.subButton2:addChild(self.soloIndicator)
  graphic:addChild(self.subButton2)

  self.subButton3 = app.TextPanel("Mute", 3, 0.25)
  self.muteIndicator = app.BinaryIndicator(0, 24, ply, 32)
  self.subButton3:addChild(self.muteIndicator)
  graphic:addChild(self.subButton3)

  branch:subscribe("contentChanged", self)
end

function SidechainMeter:onRemove()
  self.branch:unsubscribe("contentChanged", self)
  Base.onRemove(self)
end

function SidechainMeter:setDefaults(args)
  if args.map == nil then args.map = Encoder.getMap("volume") end
  if args.units == nil then args.units = app.unitDecibels end
  if args.scaling == nil then args.scaling = app.linearScaling end
  self.defaults = args
end

function SidechainMeter:getDefaults()
  return self.defaults
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
    leftOutlet = self.branch:getMonitoringOutput(1),
    rightOutlet = self.branch:getMonitoringOutput(2)
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

function SidechainMeter:mute()
  self:callUp("muteControl", self)
end

function SidechainMeter:solo()
  self:callUp("soloControl", self)
end

function SidechainMeter:isMuted()
  return self.muteIndicator:value()
end

function SidechainMeter:isSolo()
  return self.soloIndicator:value()
end

function SidechainMeter:onMuteStateChanged(muted, solo)
  local branch = self.branch
  if muted then
    self.muteIndicator:on()
    if solo == self then
      self.soloIndicator:on()
      self.fader:ungrayed()
      branch:unmute()
    else
      self.soloIndicator:off()
      self.fader:grayed()
      branch:mute()
    end
  else
    self.muteIndicator:off()
    if solo == self then
      self.soloIndicator:on()
      self.fader:ungrayed()
      branch:unmute()
    elseif solo then
      self.soloIndicator:off()
      self.fader:grayed()
      branch:mute()
    else
      self.soloIndicator:off()
      self.fader:ungrayed()
      branch:unmute()
    end
  end
end

function SidechainMeter:onInsert()
  local branch = self.branch
  if branch then
    if branch.channelCount < 2 then
      local left = branch:getMonitoringOutput(1)
      self.fader:watchOutlets(left, nil)
    else
      local left = branch:getMonitoringOutput(1)
      local right = branch:getMonitoringOutput(2)
      self.fader:watchOutlets(left, right)
    end
    self.subButton1:setText(branch:mnemonic())
  end
end

function SidechainMeter:contentChanged(chain)
  if chain == self.branch then
    if chain.channelCount < 2 then
      local left = chain:getMonitoringOutput(1)
      self.fader:watchOutlets(left, nil)
      self.scope:watchOutlet(left)
    else
      local left = chain:getMonitoringOutput(1)
      local right = chain:getMonitoringOutput(2)
      self.fader:watchOutlets(left, right)
      local Channels = require "Channels"
      if Channels.isRight() then
        self.scope:watchOutlet(right)
      else
        self.scope:watchOutlet(left)
      end
    end
    self.subButton1:setText(chain:mnemonic())
  end
end

------------------------------------------

function SidechainMeter:selectReleased(i, shifted)
  local branch = self.branch
  if branch then
    local Channels = require "Channels"
    local side = Channels.getSide(i)
    local outlet = branch:getMonitoringOutput(side)
    if outlet then self.scope:watchOutlet(outlet) end
  end
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
  if shifted then
    return false
  else
    if i == 1 then
      local branch = self.branch
      if branch then
        self:unfocus()
        branch:show()
      end
    elseif i == 2 then
      -- solo
      self:callUp("toggleSoloOnControl", self)
    elseif i == 3 then
      -- mute
      self:callUp("toggleMuteOnControl", self)
    end
  end
  return true
end

function SidechainMeter:encoder(change, shifted)
  self.fader:encoder(change, shifted, self.encoderState == Encoder.Fine)
  return true
end

return SidechainMeter
