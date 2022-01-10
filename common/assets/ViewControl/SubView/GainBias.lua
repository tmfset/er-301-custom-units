local app = app
local Class = require "Base.Class"

local Base    = require "common.assets.ViewControl.SubView"
local Chain   = require "common.assets.ViewControl.SubControl.Chain"
local Readout = require "common.assets.ViewControl.SubControl.Readout"

local GainBias = Class {}
GainBias:include(Base)

local overlay = (function ()
  local instructions = app.DrawingInstructions()

  local line1 = app.GRID5_LINE1
  local line4 = app.GRID5_LINE4

  local center3 = app.GRID5_CENTER3

  local col1 = app.BUTTON1_CENTER
  local col2 = app.BUTTON2_CENTER
  local col3 = app.BUTTON3_CENTER

  -- multiply
  instructions:circle(col2, center3, 8)
  instructions:line(col2 - 3, center3 - 3, col2 + 3, center3 + 3)
  instructions:line(col2 - 3, center3 + 3, col2 + 3, center3 - 3)

  -- sum
  instructions:circle(col3, center3, 8)
  instructions:hline(col3 - 5, col3 + 5, center3)
  instructions:vline(col3, center3 - 5, center3 + 5)

  -- arrow: branch to gain
  instructions:hline(col1 + 20, col2 - 9, center3)
  instructions:triangle(col2 - 12, center3, 0, 3)

  -- arrow: gain to bias
  instructions:hline(col2 + 9, col3 - 8, center3)
  instructions:triangle(col3 - 11, center3, 0, 3)

  -- arrow: bias to title
  instructions:vline(col3, center3 + 8, line1 - 2)
  instructions:triangle(col3, line1 - 2, 90, 3)

  return instructions
end)()

function GainBias:addDrawing()
  local drawing = app.Drawing(0, 0, 128, 64)
  drawing:add(overlay)
  self.graphic:addChild(drawing)
end

function GainBias:init(args)
  Base.init(self, args)

  self:addDrawing()

  Chain {
    parent   = self,
    position = 1,
    name     = "empty",
    branch   = args.branch,
    column   = app.BUTTON1_CENTER - 20,
    row      = app.GRID5_LINE4
  }

  Readout {
    parent        = self,
    position      = 2,
    name          = "gain",
    parameter     = args.gainBias:getParameter("Gain"),
    encoderMap    = args.gainEncoderMap,
    units         = args.units,
    precision     = args.precision,
    editMessage   = string.format("'%s' modulation gain.", self.name),
    commitMessage = string.format("'%s' gain updated.", self.name),
    column        = app.BUTTON2_CENTER,
    row           = app.GRID5_CENTER4
  }

  Readout {
    parent        = self,
    position      = 3,
    name          = "bias",
    parameter     = args.gainBias:getParameter("Bias"),
    encoderMap    = args.biasEncoderMap,
    units         = args.units,
    precision     = args.precision,
    editMessage   = string.format("'%s' modulation bias.", self.name),
    commitMessage = string.format("'%s' bias updated.", self.name),
    column        = app.BUTTON3_CENTER,
    row           = app.GRID5_CENTER4
  }
end

function GainBias:onFocused()
  self:setFocusedPosition(3)
  Base.onFocused(self)
end

return GainBias
