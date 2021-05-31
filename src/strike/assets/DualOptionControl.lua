local app = app
local strike = require "strike.libstrike"
local Drawings = require "Drawings"
local Utils = require "Utils"
local Class = require "Base.Class"
local ViewControl = require "Unit.ViewControl"

local ply = app.SECTION_PLY

local DualOptionControl = Class {
  type = "Option",
  canEdit = false,
  canMove = true
}
DualOptionControl:include(ViewControl)

function DualOptionControl:init(args)
  ViewControl.init(self)
  self:setClassName("strike.DualOptionControl")

  local button = args.button or app.logError("%s.init: button is missing.", self)
  self:setInstanceName(button)

  local optionA  = args.optionA or app.logError("%s.init: optionA is missing.", self)
  local choicesA = args.choicesA or app.logError("%s.init: choicesA is missing.", self)
  self.optionA   = optionA
  self.choicesA  = choicesA

  local optionB  = args.optionB or app.logError("%s.init: optionB is missing.", self)
  local choicesB = args.choicesB or app.logError("%s.init: choicesB is missing.", self)
  self.optionB   = optionB
  self.choicesB  = choicesB

  optionA:enableSerialization()
  optionB:enableSerialization()

  self.muteOnChange = args.muteOnChange

  local graphic = app.Graphic(0, 0, ply, 64)
  self.graphic = graphic

  local yOffset = 5

  self.labelDescA = (function ()
    local text = args.descriptionA or app.logError("%s.init: descriptionA is missing.", self)
    local label = app.Label(text, 10)
    graphic:addChild(label)
    label:setPosition(0, app.GRID5_LINE1 - yOffset)
    label:setSize(ply, label.mHeight)
    return label
  end)()

  self.labelChoiceA = (function ()
    local text = choicesA[optionA:value()]
    local label = app.Label(text, 10)
    graphic:addChild(label)
    label:setPosition(0, app.GRID5_LINE2 - yOffset)
    label:setSize(ply, label.mHeight)
    return label
  end)()

  self.labelDescA = (function ()
    local text = args.descriptionB or app.logError("%s.init: descriptionB is missing.", self)
    local label = app.Label(text, 10)
    graphic:addChild(label)
    label:setPosition(0, app.GRID5_LINE3 - yOffset * 2)
    label:setSize(ply, label.mHeight)
    return label
  end)()

  self.labelChoiceB = (function ()
    local text = choicesB[optionB:value()]
    local label = app.Label(text, 10)
    graphic:addChild(label)
    label:setPosition(0, app.GRID5_LINE4 - yOffset * 2)
    label:setSize(ply, label.mHeight)
    return label
  end)()

  self:setControlGraphic(self.graphic)
  self:addSpotDescriptor {
    center = 0.5 * ply
  }

  local subGraphic = strike.MultiOptionView()
  self.subGraphic = subGraphic

  for i,v in ipairs(choicesB) do
    subGraphic:addChoice(0, v)
  end
  subGraphic:setLaneSelection(0, optionB:value() - 1)

  for i,v in ipairs(choicesA) do
    subGraphic:addChoice(1, v)
  end
  subGraphic:setLaneSelection(1, optionA:value() - 1)

  subGraphic:setActiveLane(1)
end

function DualOptionControl:subPressed(i, shifted)
  local option
  if self.subGraphic:getActiveLane() == 0 then
    option = self.optionB
  else
    option = self.optionA
  end

  local chain = self.parent.chain
  local wasMuted
  if self.muteOnChange then
    wasMuted = chain:muteIfNeeded()
  end

  option:set(i)
  self:update()

  if self.muteOnChange then
    chain:unmuteIfNeeded(wasMuted)
  end

  return true
end

function DualOptionControl:update()
  self.subGraphic:setLaneSelection(1, self.optionA:value() - 1)
  self.subGraphic:setLaneSelection(0, self.optionB:value() - 1)

  self.labelChoiceA:setText(self.choicesA[self.optionA:value()])
  self.labelChoiceB:setText(self.choicesB[self.optionB:value()])
end

function DualOptionControl:onCursorEnter(spot)
  ViewControl.onCursorEnter(self, spot)
  self:grabFocus("shiftPressed", "shiftReleased", "enterReleased")
end

function DualOptionControl:onCursorLeave(spot)
  ViewControl.onCursorLeave(self, spot)
  self:releaseFocus("shiftPressed", "shiftReleased", "enterReleased")
end

function DualOptionControl:shiftPressed()
  self.subGraphic:advanceActiveLane()
  return false
end

return DualOptionControl
