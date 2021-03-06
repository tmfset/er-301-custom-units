local app = app
local Class = require "Base.Class"
local Base = require "Unit.ViewControl.EncoderControl"
local Encoder = require "Encoder"
local ply = app.SECTION_PLY

local line1 = app.GRID5_LINE1
-- local line2 = app.GRID5_LINE2
-- local line3 = app.GRID5_LINE3
local line4 = app.GRID5_LINE4
local center1 = app.GRID5_CENTER1
-- local center2 = app.GRID5_CENTER2
local center3 = app.GRID5_CENTER3
local center4 = app.GRID5_CENTER4
local col1 = app.BUTTON1_CENTER
local col2 = app.BUTTON2_CENTER
local col3 = app.BUTTON3_CENTER

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

  local description = args.description or app.logError("%s.init: description is missing.", self)
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

  self.subGraphic = app.Graphic(0, 0, 128, 64)
  self.subGraphic:addChild(app.SubButton("side", 1))
  self.subGraphic:addChild(app.SubButton("enable", 2))
  self.subGraphic:addChild(app.SubButton("gain", 3))

  self.description = (function ()
    local desc = app.Label(description, 10)
    desc:fitToText(3)
    desc:setSize(ply * 2, desc.mHeight)
    desc:setBorder(1)
    desc:setCornerRadius(3, 0, 0, 3)
    desc:setCenter(0.5 * (col2 + col3), center1 + 1)
    return desc
  end)()
  self.subGraphic:addChild(self.description)

  self.scope = (function ()
    local scope = app.MiniScope(col1 - 20, line4, 40, 45)
    scope:setBorder(1)
    scope:setCornerRadius(3, 3, 3, 3)
    return scope
  end)()
  self.subGraphic:addChild(self.scope)

  local drawing = (function ()
    local instructions = app.DrawingInstructions()
    instructions:hline(col1 + 20, col2 - 10, center3)

    instructions:circle(col3, center3, 8)
    instructions:hline(col3 - 5, col3 + 5, center3)
    instructions:vline(col3, center3 - 5, center3 + 5)

    instructions:vline(col3, center3 + 8, line1 - 2)
    instructions:triangle(col3, line1 - 2, 90, 3)

    local drawing = app.Drawing(0, 0, 128, 64)
    drawing:add(instructions)
    return drawing
  end)()
  self.subGraphic:addChild(drawing)

  self.enableIndicator = (function ()
    local option = compressor:getOption("Enable Sidechain")
    option:enableSerialization()
    local ind = app.BinaryIndicator(0, 24, ply, 32)
    ind:setCenter(col2, center3);
    return ind
  end)()
  self.subGraphic:addChild(self.enableIndicator)

  self.inputGain = (function ()
    local param = compressor:getParameter("Input Gain")
    param:enableSerialization()

    local readout = app.Readout(0, 0, ply, 10)
    readout:setParameter(param)
    readout:setAttributes(defaults.units, defaults.map)
    readout:setCenter(col3, center4)
    return readout
  end)()
  self.subGraphic:addChild(self.inputGain)

  branch:subscribe("contentChanged", self)
end

function SidechainMeter:onCursorEnter()
  self:updateViewState()
  return Base.onCursorEnter(self)
end

function SidechainMeter:onRemove()
  self.branch:unsubscribe("contentChanged", self)
  Base.onRemove(self)
end

function SidechainMeter:getPinControl()
  local Fader = require "PinView.Fader"
  return Fader {
    delegate    = self,
    name        = self.fader:getLabel(),
    valueParam  = self.fader:getValueParameter(),
    range       = self.fader:getRangeObject(),
    units       = self.defaults.units,
    map         = self.defaults.map,
    scaling     = self.defaults.scaling,
    precision   = 1,
    leftOutlet  = self:getLeftWatch(),
    rightOutlet = self:getRightWatch()
  }
end

function SidechainMeter:createPinMark()
  local offset
  if self:getRightWatch() then
    offset = 0
  else
    offset = -3
  end
  local Drawings = require "Drawings"
  local graphic = app.Drawing(offset, 0, app.SECTION_PLY, 64)
  graphic:add(Drawings.Control.Pin)
  self.controlGraphic:addChildOnce(graphic)
  self.pinMark = graphic
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
