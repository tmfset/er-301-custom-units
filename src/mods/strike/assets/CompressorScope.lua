local app = app
local strike = require "strike.libstrike"
local Env = require "Env"
local Class = require "Base.Class"
local ViewControl = require "Unit.ViewControl"
local Encoder = require "Encoder"
local ply = app.SECTION_PLY

local ply = app.SECTION_PLY
local line1 = app.GRID5_LINE1
local line4 = app.GRID5_LINE4
local center1 = app.GRID5_CENTER1
local center2 = app.GRID5_CENTER2
local center3 = app.GRID5_CENTER3
local center4 = app.GRID5_CENTER4
local col1 = app.BUTTON1_CENTER
local col2 = app.BUTTON2_CENTER
local col3 = app.BUTTON3_CENTER

local CompressorScope = Class {}
CompressorScope:include(ViewControl)

local function linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

function CompressorScope:init(args)
  ViewControl.init(self)
  self:setClassName("strike.CompressorScope")
  local width = args.width or ply
  local compressor = args.compressor or app.logError("%s.init: compressor is missing.", self)
  local description = args.description or ""

  -- add spots
  for i = 1, (width // ply) do
    self:addSpotDescriptor{
      center = (i - 0.5) * ply
    }
  end
  self.verticalDivider = width

  self.scope = strike.CompressorScope(0, 0, width, 64)
  self.scope:watch(compressor)
  self:setMainCursorController(self.scope)
  self:setControlGraphic(self.scope)

  self.ratio = {
    readout = (function ()
      local graphic = app.Readout(0, 0, ply, 10)
      local param = compressor:getParameter("Ratio")
      param:enableSerialization()
      graphic:setParameter(param)
      graphic:setAttributes(app.unitNone, linMap(1, 40, 1, 0.1, 0.01, 0.001))
      graphic:setPrecision(2)
      graphic:setCenter(col1, center4)
      return graphic
    end)(),
    message       = "Compression ratio.",
    commitMessage = "Updated compression ratio.",
    encoderState  = Encoder.Coarse
  }

  self.rise = {
    readout = (function ()
      local graphic = app.Readout(0, 0, ply, 10)
      local param = compressor:getParameter("Rise")
      param:enableSerialization()
      graphic:setParameter(param)
      graphic:setAttributes(app.unitSecs, linMap(0, 0.250, 0.1, 0.01, 0.001, 0.001))
      graphic:setPrecision(3)
      graphic:setCenter(col2, center4)
      return graphic
    end)(),
    message       = "Compression rise time.",
    commitMessage = "Updated rise time.",
    encoderState  = Encoder.Fine
  }

  self.fall = {
    readout = (function ()
      local graphic = app.Readout(0, 0, ply, 10)
      local param = compressor:getParameter("Fall")
      param:enableSerialization()
      graphic:setParameter(param)
      graphic:setAttributes(app.unitSecs, linMap(0, 2, 0.1, 0.01, 0.001, 0.001))
      graphic:setPrecision(3)
      graphic:setCenter(col3, center4)
      return graphic
    end)(),
    message       = "Compression fall time.",
    commitMessage = "Updated fall time.",
    encoderState  = Encoder.Fine
  }

  self.description = (function ()
    local graphic = app.Label(description, 10)
    graphic:fitToText(3)
    graphic:setSize(ply * 3, graphic.mHeight)
    graphic:setBorder(1)
    graphic:setCornerRadius(3, 0, 0, 3)
    graphic:setCenter(col2, center1 + 1)
    return graphic
  end)()

  -- sub display
  self.subGraphic = app.Graphic(0, 0, 128, 64)
  self.subGraphic:addChild(self.ratio.readout)
  self.subGraphic:addChild(self.rise.readout)
  self.subGraphic:addChild(self.fall.readout)
  self.subGraphic:addChild(self.description)
  self.subGraphic:addChild(app.SubButton("ratio", 1))
  self.subGraphic:addChild(app.SubButton("rise",  2))
  self.subGraphic:addChild(app.SubButton("fall",  3))
end

function CompressorScope:setFocusedReadout(args)
  self.focusedReadout = args
  if self.focusedReadout then
    self.focusedReadout.readout:save()
    Encoder.set(self.focusedReadout.encoderState)
    self:setSubCursorController(self.focusedReadout.readout)
  else
    Encoder.set(self.encoderState or Encoder.Horizontal)
  end
end

function CompressorScope.switchEncoderState(before)
  if before == Encoder.Coarse then return Encoder.Fine
  else                             return Encoder.Coarse end
end

function CompressorScope:dialPressed(shifted)
  if self.focusedReadout then
    self.focusedReadout.encoderState = self.switchEncoderState(self.focusedReadout.encoderState)
    Encoder.set(self.focusedReadout.encoderState)
    return true
  end

  return false
end

function CompressorScope:dialReleased(shifted)
  if self.focusedReadout then
    return true
  end

  return false
end

function CompressorScope:zeroPressed()
  if self.focusedReadout then self.focusedReadout.readout:zero() end
  return true
end

function CompressorScope:cancelReleased(shifted)
  if self.focusedReadout then self.focusedReadout.readout:restore() end
  return true
end

function CompressorScope:doKeyboardSet(args)
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

function CompressorScope:spotReleased(spot, shifted)
  if self.focusedReadout then
    self:setFocusedReadout(nil)
  end

  return false
end

function CompressorScope:subReleased(i, shifted)
  local focused = nil;
  if     i == 1 then focused = self.ratio
  elseif i == 2 then focused = self.rise
  elseif i == 3 then focused = self.fall end

  if focused then
    if self:hasFocus("encoder") then
      local isAlreadyFocused = self.focusedReadout == focused
      if isAlreadyFocused then self:doKeyboardSet(focused)
      else                     self:setFocusedReadout(focused) end
    else
      self:focus()
      self:setFocusedReadout(focused)
    end
  end

  return true
end

function CompressorScope:encoder(change, shifted)
  if self.focusedReadout then
    self.focusedReadout.readout:encoder(change, shifted, self.focusedReadout.encoderState == Encoder.Fine)
    return true
  end

  return false
end

return CompressorScope
