local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Base = require "Unit.ViewControl.EncoderControl"
local Encoder = require "Encoder"
local UnitShared = require "shared.UnitShared"

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

local EuclidCircle = Class {
  type    = "EuclidCircle",
  canEdit = false,
  canMove = true
}
EuclidCircle:include(Base)
EuclidCircle:include(UnitShared)

function EuclidCircle:init(args)
  local description = args.description or app.logError("%s.init: description is missing.", self)
  local euclid      = args.euclid or app.logError("%s.init: euclid is missing.", self)
  local max         = args.max or app.logError("%s.init: max is missing.", self)

  Base.init(self, "circle")
  self:setClassName("EuclidCircle")

  local width = args.width or (2 * ply)

  local graphic = app.Graphic(0, 0, width, 64)
  self.pDisplay = lojik.EuclidCircle(0, 0, width, 64)
  graphic:addChild(self.pDisplay)
  self:setMainCursorController(self.pDisplay)
  self:setControlGraphic(graphic)

  -- add spots
  for i = 1, (width // ply) do
    self:addSpotDescriptor {
      center = (i - 0.5) * ply
    }
  end

  self.beats = (function ()
    local graphic = app.Readout(0, 0, ply, 10)
    local param = args.beats or euclid:getParameter("Beats")
    param:enableSerialization()
    graphic:setParameter(param)
    graphic:setAttributes(app.unitNone, self.intMap(0, max))
    graphic:setPrecision(0)
    graphic:setCenter(col1, center4)
    return graphic
  end)()

  self.length = (function ()
    local graphic = app.Readout(0, 0, ply, 10)
    local param = args.length or euclid:getParameter("Length")
    param:enableSerialization()
    graphic:setParameter(param)
    graphic:setAttributes(app.unitNone, self.intMap(1, max))
    graphic:setPrecision(0)
    graphic:setCenter(col2, center4)
    return graphic
  end)()

  self.rotate = (function ()
    local graphic = app.Readout(0, 0, ply, 10)
    local param = args.rotate or euclid:getParameter("Shift")
    param:enableSerialization()
    graphic:setParameter(param)
    graphic:setAttributes(app.unitNone, self.intMap(-(max / 2), (max / 2)))
    graphic:setPrecision(0)
    graphic:setCenter(col3, center4)
    return graphic
  end)()

  self.description = (function ()
    local graphic = app.Label(description, 10)
    graphic:fitToText(3)
    graphic:setSize(ply * 3, graphic.mHeight)
    graphic:setBorder(1)
    graphic:setCornerRadius(3, 0, 0, 3)
    graphic:setCenter(col2, center1 + 1)
    return graphic
  end)()

  local art = (function ()
    local draw = app.DrawingInstructions()
    -- draw:vline(col1, center3, line1 - 2)
    -- draw:vline(col2, center3, line1 - 2)
    -- draw:vline(col3, center3, line1 - 2)

    -- draw:triangle(col1, line1 - 2, 90, 3)
    -- draw:triangle(col2, line1 - 2, 90, 3)
    -- draw:triangle(col3, line1 - 2, 90, 3)

    local graphic = app.Drawing(0, 0, 128, 64)
    graphic:add(draw)
    return graphic
  end)()

  self.subGraphic = app.Graphic(0, 0, 128, 64)
  self.subGraphic:addChild(art)
  self.subGraphic:addChild(self.beats)
  self.subGraphic:addChild(self.length)
  self.subGraphic:addChild(self.rotate)
  self.subGraphic:addChild(self.description)
  self.subGraphic:addChild(app.SubButton("beats",  1))
  self.subGraphic:addChild(app.SubButton("length", 2))
  self.subGraphic:addChild(app.SubButton("rotate", 3))

  self:follow(euclid)
end

function EuclidCircle:follow(euclid)
  self.pDisplay:follow(euclid)
  self.euclid = euclid
end

function EuclidCircle:setFocusedReadout(readout)
  if readout then readout:save() end
  self.focusedReadout = readout
  self:setSubCursorController(readout)
end

function EuclidCircle:zeroPressed()
  if self.focusedReadout then self.focusedReadout:zero() end
  return true
end

function EuclidCircle:cancelReleased(shifted)
  if self.focusedReadout then self.focusedReadout:restore() end
  return true
end

function EuclidCircle:doKeyboardSet(args)
  local Decimal = require "Keyboard.Decimal"

  local keyboard = Decimal {
    message       = args.message,
    commitMessage = args.commit,
    initialValue  = args.selected:getValueInUnits(),
    integerOnly   = true
  }

  local task = function(value)
    if value then
      args.selected:save()
      args.selected:setValueInUnits(value)
      self:unfocus()
    end
  end

  keyboard:subscribe("done", task)
  keyboard:subscribe("commit", task)
  keyboard:show()
end

function EuclidCircle:subReleased(i, shifted)
  if shifted then return false end

  local args = nil;
  if i == 1 then
    args = {
      selected = self.beats,
      message  = "Number of beats.",
      commit   = "Updated beats."
    }
  elseif i == 2 then
    args = {
      selected = self.length,
      message  = "Total number of steps.",
      commit   = "Updated length."
    }
  elseif i == 3 then
    args = {
      selected = self.rotate,
      message  = "Rotate pattern.",
      commit   = "Updated rotate."
    }
  end

  if args then
    if self:hasFocus("encoder") then
      local alreadyFocused = self.focusedReadout == args.selected
      if alreadyFocused then self:doKeyboardSet(args)
      else                   self:setFocusedReadout(args.selected) end
    else
      self:focus()
      self:setFocusedReadout(args.selected)
    end
  end

  return true
end

function EuclidCircle:encoder(change, shifted)
  if self.focusedReadout then
    self.focusedReadout:encoder(change, shifted, self.encoderState == Encoder.Coarse)
  end
  return true
end

return EuclidCircle