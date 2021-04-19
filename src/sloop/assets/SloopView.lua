local app = app
local sloop = require "sloop.libsloop"
local Class = require "Base.Class"
local Zoomable = require "Unit.ViewControl.Zoomable"
local Channels = require "Channels"
local Encoder = require "Encoder"
local Signal = require "Signal"

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

local SloopView = Class {}
SloopView:include(Zoomable)

function SloopView:init(args)
  Zoomable.init(self)
  self:setClassName("SloopView")
  local width = args.width or (4 * ply)
  local head = args.head or app.logError("%s: head is missing from args.", self)
  local description = args.description or ""
  self.head = head

  local graphic
  graphic = app.Graphic(0, 0, width, 64)
  self.mainDisplay = sloop.SloopHeadMainDisplay(head, 0, 0, width, 64)
  graphic:addChild(self.mainDisplay)
  self:setMainCursorController(self.mainDisplay)
  self:setControlGraphic(graphic)

  -- add spots
  for i = 1, (width // ply) do
    self:addSpotDescriptor{
      center = (i - 0.5) * ply
    }
  end
  self.verticalDivider = width

  self.through = {
    readout      = (function ()
      local graphic = app.Readout(0, 0, ply, 10)
      local param = args.beats or head:getParameter("Through")
      param:enableSerialization()
      graphic:setParameter(param)
      graphic:setAttributes(app.unitNone, Encoder.getMap("[0,1]"))
      graphic:setPrecision(2)
      graphic:setCenter(col1, center4)
      return graphic
    end)(),
    message       = "Input through level.",
    commitMessage = "Updated through level.",
    encoderState  = Encoder.Fine
  }

  self.fadeIn = {
    readout       = (function ()
      local graphic = app.Readout(0, 0, ply, 10)
      local param = args.length or head:getParameter("Fade In")
      param:enableSerialization()
      graphic:setParameter(param)
      graphic:setAttributes(app.unitSecs, Encoder.getMap("[0,10]"))
      graphic:setPrecision(3)
      graphic:setCenter(col2, center4)
      return graphic
    end)(),
    message       = "Recording fade in time in seconds.",
    commitMessage = "Updated fade in.",
    encoderState  = Encoder.Fine
  }

  self.fadeOut = {
    readout = (function ()
      local graphic = app.Readout(0, 0, ply, 10)
      local param = args.rotate or head:getParameter("Fade Out")
      param:enableSerialization()
      graphic:setParameter(param)
      graphic:setAttributes(app.unitSecs, Encoder.getMap("[0,10]"))
      graphic:setPrecision(3)
      graphic:setCenter(col3, center4)
      return graphic
    end)(),
    message       = "Recording fade out time in seconds.",
    commitMessage = "Updated fade out.",
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

  self.headSub = sloop.SloopHeadSubDisplay(head);

  -- sub display
  self.subGraphic = app.Graphic(0, 0, 128, 64)
  self.subGraphic:addChild(self.through.readout)
  self.subGraphic:addChild(self.fadeIn.readout)
  self.subGraphic:addChild(self.fadeOut.readout)
  self.subGraphic:addChild(self.description)
  self.subGraphic:addChild(self.headSub)
  self.subGraphic:addChild(app.SubButton("through",  1))
  self.subGraphic:addChild(app.SubButton("fade in",  2))
  self.subGraphic:addChild(app.SubButton("fade out", 3))
end

function SloopView:setSample(sample, displayName)
  if sample then
    self.description:setText(displayName)
    self.mainDisplay:setChannel(Channels.getSide() - 1)
  end
end

function SloopView:selectReleased(i, shifted)
  self.mainDisplay:setChannel(Channels.getSide(i) - 1)
  return true
end

function SloopView:setFocusedReadout(args)
  self.focusedReadout = args
  if self.focusedReadout then
    self.focusedReadout.readout:save()
    Encoder.set(self.focusedReadout.encoderState)
    self:setSubCursorController(self.focusedReadout.readout)
  else
    Encoder.set(self.encoderState or Encoder.Horizontal)
  end
end

function SloopView.switchEncoderState(before)
  if before == Encoder.Coarse then return Encoder.Fine
  else                             return Encoder.Coarse end
end

function SloopView:dialPressed(shifted)
  if self.focusedReadout then
    self.focusedReadout.encoderState = self.switchEncoderState(self.focusedReadout.encoderState)
    Encoder.set(self.focusedReadout.encoderState)
    return true
  end

  return Zoomable.dialPressed(self, shifted)
end

function SloopView:dialReleased(shifted)
  if self.focusedReadout then
    return true
  end

  return Zoomable.dialReleased(self, shifted)
end

function SloopView:zeroPressed()
  if self.focusedReadout then self.focusedReadout.readout:zero() end
  return true
end

function SloopView:cancelReleased(shifted)
  if self.focusedReadout then self.focusedReadout.readout:restore() end
  return true
end

function SloopView:doKeyboardSet(args)
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

function SloopView:getFloatingMenuItems()
  local choices = Zoomable.getFloatingMenuItems(self)
  choices[#choices + 1] = "open editor"
  return choices
end

function SloopView:onFloatingMenuSelection(choice)
  if choice == "open editor" then
    self:callUp("showSampleEditor")
    return true
  end

  return Zoomable.onFloatingMenuSelection(self, choice)
end

function SloopView:spotReleased(spot, shifted)
  if self.focusedReadout then
    self:setFocusedReadout(nil)
  end

  return false
end

function SloopView:subReleased(i, shifted)
  local focused = nil;
  if     i == 1 then focused = self.through
  elseif i == 2 then focused = self.fadeIn
  elseif i == 3 then focused = self.fadeOut end

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

function SloopView:encoder(change, shifted)
  if self.focusedReadout then
    self.focusedReadout.readout:encoder(change, shifted, self.focusedReadout.encoderState == Encoder.Fine)
    return true
  end

  return Zoomable.encoder(self, change, shifted)
end

function SloopView:serialize()
  local t = Zoomable.serialize(self)
  return t
end

function SloopView:deserialize(t)
  Zoomable.deserialize(self, t)
end

return SloopView
