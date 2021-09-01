local app = app
local Class = require "Base.Class"
local Encoder = require "Encoder"
local SubView = require "polygon.SubView"
local ply = app.SECTION_PLY

local line1 = app.GRID5_LINE1
local line4 = app.GRID5_LINE4

local center1 = app.GRID5_CENTER1
local center3 = app.GRID5_CENTER3
local center4 = app.GRID5_CENTER4

local col1 = app.BUTTON1_CENTER
local col2 = app.BUTTON2_CENTER
local col3 = app.BUTTON3_CENTER

local GateSubView = Class {}
GateSubView:include(SubView)

local overlay = (function ()
  local instructions = app.DrawingInstructions()

  -- threshold
  instructions:box(col2 - 13, center3 - 8, 26, 16)
  instructions:startPolyline(col2 - 8, center3 - 4, 0)
  instructions:vertex(col2, center3 - 4)
  instructions:vertex(col2, center3 + 4)
  instructions:endPolyline(col2 + 8, center3 + 4)
  instructions:color(app.GRAY3)
  instructions:hline(col2 - 9, col2 + 9, center3)
  instructions:color(app.WHITE)

  -- or
  instructions:circle(col3, center3, 8)

  -- arrow: branch to thresh
  instructions:hline(col1 + 20, col2 - 13, center3)
  instructions:triangle(col2 - 16, center3, 0, 3)

  -- arrow: thresh to or
  instructions:hline(col2 + 13, col3 - 8, center3)
  instructions:triangle(col3 - 11, center3, 0, 3)

  -- arrow: or to title
  instructions:vline(col3, center3 + 8, line1 - 2)
  instructions:triangle(col3, line1 - 2, 90, 3)

  -- arrow: fire to or
  instructions:vline(col3, line4, center3 - 8)
  instructions:triangle(col3, center3 - 11, 90, 3)

  return instructions
end)()

function GateSubView:init(args)
  local description = args.description or app.logError("%s.init: missing description.", self)

  local branch = args.branch or app.logError("%s.init: missing branch.", self)
  self.branch = branch
  self.branch:subscribe("contentChanged", self)

  local threshold = args.threshold or app.logError("%s.init: missing threshold paramater.", self)
  threshold:enableSerialization()

  local fire = args.fire or app.logError("%s.init: missing fire callback.", self)
  self.fire = fire

  self.graphic = app.Graphic(0, 0, 128, 64)

  local drawing = app.Drawing(0, 0, 128, 64)
  drawing:add(overlay)
  self.graphic:addChild(drawing)

  local label = app.Label("or", 10)
  label:fitToText(0)
  label:setCenter(col3 + 1, center3 + 1)
  self.graphic:addChild(label)

  self.scope = app.MiniScope(col1 - 20, line4, 40, 45)
  self.scope:setBorder(1)
  self.scope:setCornerRadius(3, 3, 3, 3)
  self.graphic:addChild(self.scope)

  self.threshold = {
    readout = (function ()
      local readout = app.Readout(0, 0, ply, 10)
      readout:setParameter(threshold)
      readout:setAttributes(app.unitNone, Encoder.getMap("default"))
      readout:setCenter(col2, center4)
      return readout
    end)(),
    message       = "Gate detection threshold.",
    commitMessage = "Updated gate detection threshold.",
    encoderState  = Encoder.Coarse
  }
  self.graphic:addChild(self.threshold.readout)

  self.description = app.Label(description, 10)
  self.description:fitToText(3)
  self.description:setSize(ply * 2 - 4, self.description.mHeight)
  self.description:setBorder(1)
  self.description:setCornerRadius(3, 3, 3, 3)
  self.description:setCenter(0.5 * (col2 + col3), center1)
  self.graphic:addChild(self.description)

  self.modButton = app.SubButton("empty", 1)
  self.graphic:addChild(self.modButton)

  self.threshButton = app.SubButton("thresh", 2)
  self.graphic:addChild(self.threshButton)

  self.fireButton = app.SubButton("fire", 3)
  self.graphic:addChild(self.fireButton)
end

function GateSubView:onRemove()
  self.branch:unsubscribe("contentChanged", self)
end

function GateSubView:contentChanged(chain)
  if chain ~= self.branch then return end

  local outlet = chain:getMonitoringOutput(1)
  self.scope:watchOutlet(outlet)
  self.modButton:setText(chain:mnemonic())
end

function GateSubView:getReadoutByIndex(i)
  if i == 2 then return self.threshold end
  return nil
end

function GateSubView:subReleased(i)
  if i == 1 and self.branch then
    self:unfocus()
    self.branch:show()
  end

  if i == 2 then
    self:setFocusedReadout(self.threshold)
  end

  if i == 3 then
    -- simulate falling edge?
  end

  return true
end

function GateSubView:subPressed(i)
  if i == 3 then
    self.fire()
  end
  
  return true
end

return GateSubView