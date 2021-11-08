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

local OutputMeter = Class {
  type = "Meter",
  canEdit = false,
  canMove = false
}
OutputMeter:include(Base)

function OutputMeter:init(args)
  Base.init(self)
  self:setClassName("strike.OutputMeter")
  local button = args.button or app.logError("%s.init: button is missing.", self)
  self:setInstanceName(button)

  local description = args.description or app.logError("%s.init: description is missing.", self)
  local compressor = args.compressor or app.logError("%s.init: compressor is missing.", self)
  local channelCount = args.channelCount or app.logError("%s.init: channelCount is missing.", self)

  local defaults = {
    map = args.map or app.logError("%s.init: map is missing.", self),
    units = args.units or app.logError("%s.init: units is missing.", self),
    scaling = args.scaling or app.logError("%s.init: scaling is missing.", self)
  }

  self.defaults = defaults

  self.compressor = compressor
  self.channelCount = channelCount

  self.fader = (function ()
    local fader = app.Fader(0, 0, ply, 64)
    fader:setParameter(compressor:getParameter("Output Gain"))
    fader:setLabel(button)
    fader:setAttributes(defaults.units, defaults.map, defaults.scaling)
    fader:setTextBelow(-59, "-inf dB")
    fader:setPrecision(1)
    return fader
  end)()

  self.fader:watchOutlets(self:getLeftWatch(), self:getRightWatch())

  self:setMainCursorController(self.fader)
  self:setControlGraphic(self.fader)
  self:addSpotDescriptor {
    center = 0.5 * ply
  }

  local graphic = app.Graphic(0, 0, 128, 64)
  self.subGraphic = graphic
  self.subGraphic:addChild(app.SubButton("auto", 2))
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

  local readout = (function ()
    local param = compressor:getParameter("Makeup")
    local graphic = app.Readout(0, 0, ply, 10);
    graphic:setParameter(param);
    graphic:setPrecision(1)
    graphic:setCenter(col1, center3 + 2)
    return graphic
  end)()
  self.subGraphic:addChild(readout);

  local drawing = (function ()
    local instructions = app.DrawingInstructions()
    instructions:hline(col1 + 12, col2 - 10, center3)
    instructions:hline(col2 + 10, col3 - 10, center3)
    instructions:triangle(col3 - 11, center3, 0, 3)

    instructions:box(col1 - 12, center3 - 12, 24, 24);

    instructions:circle(col3, center3, 8)
    instructions:hline(col3 - 5, col3 + 5, center3)
    instructions:vline(col3, center3 - 5, center3 + 5)

    instructions:vline(col3, center3 + 8, line1 - 2)
    instructions:triangle(col3, line1 - 2, 90, 3)

    local d = app.Drawing(0, 0, 128, 64)
    d:add(instructions)
    return d
  end)()
  self.subGraphic:addChild(drawing)

  self.autoMakeupIndicator = (function ()
    local option = compressor:getOption("Auto Makeup Gain")
    option:enableSerialization()
    local ind = app.BinaryIndicator(0, 24, ply, 32)
    ind:setCenter(col2, center3)
    return ind
  end)()
  self.subGraphic:addChild(self.autoMakeupIndicator)

  self.inputGain = (function ()
    local param = compressor:getParameter("Output Gain")
    param:enableSerialization()

    local readout = app.Readout(0, 0, ply, 10)
    readout:setParameter(param)
    readout:setAttributes(defaults.units, defaults.map)
    readout:setCenter(col3, center4)
    return readout
  end)()
  self.subGraphic:addChild(self.inputGain)
end

function OutputMeter:onCursorEnter()
  self:updateViewState()
  return Base.onCursorEnter(self)
end

function OutputMeter:getPinControl()
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

function OutputMeter:createPinMark()
  local offset
  if self.channelCount < 2 then
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

function OutputMeter:isMuted()
  return self.muteIndicator:value()
end

function OutputMeter:isSolo()
  return self.soloIndicator:value()
end

function OutputMeter:getLeftWatch()
  return self.compressor:getOutput("Out1")
end

function OutputMeter:getRightWatch()
  if self.channelCount < 2 then
    return nil
  end

  return self.compressor:getOutput("Out2")
end

function OutputMeter:updateViewState()
  if self.compressor:isAutoMakeupEnabled() then
    self.autoMakeupIndicator:on()
  else
    self.autoMakeupIndicator:off()
  end
end

function OutputMeter:onInsert()
  self:updateViewState()
end

function OutputMeter:selectReleased(i, shifted)
  self:updateViewState()
  return true
end

function OutputMeter:zeroPressed()
  self.fader:zero()
  return true
end

function OutputMeter:cancelReleased(shifted)
  if not shifted then self.fader:restore() end
  return true
end

function OutputMeter:onFocused()
  self.fader:save()
end

function OutputMeter:subReleased(i, shifted)
  if shifted then return false end

  if i == 2 then
    self.compressor:toggleAutoMakeup()
    self:updateViewState()
  end

  return true
end

function OutputMeter:encoder(change, shifted)
  self.fader:encoder(change, shifted, self.encoderState == Encoder.Fine)
  return true
end

return OutputMeter
