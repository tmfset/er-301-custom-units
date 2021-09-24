local app     = app
local Class   = require "Base.Class"
local Encoder = require "Encoder"

local Base           = require "polygon.SubView"
local SubViewReadout = require "polygon.SubViewReadout"
local SubViewChain   = require "polygon.SubViewChain"

local col2 = app.BUTTON2_CENTER
local col3 = app.BUTTON3_CENTER
local col2h = col2 + (col3 - col2) / 2

local PitchSubView = Class {}
PitchSubView:include(Base)

local overlay = (function ()
  local instructions = app.DrawingInstructions()

  local line1 = app.GRID5_LINE1
  local center3 = app.GRID5_CENTER3
  local col1 = app.BUTTON1_CENTER

  -- sum
  instructions:circle(col2, center3, 8)
  instructions:hline(col2 - 5, col2 + 5, center3)
  instructions:vline(col2, center3 - 5, center3 + 5)

  -- 2^V
  instructions:box(col2h - 8, center3 - 8, 16, 16)

  -- arrow: branch to bias
  instructions:hline(col1 + 20, col2 - 9, center3)
  instructions:triangle(col2 - 12, center3, 0, 3)

  -- arrow: bias to 2^V
  instructions:hline(col2 + 9, col2h - 8, center3)
  --instructions:triangle(col2h - 11, center3, 0, 3)

  -- arrow: 2^V to title
  instructions:vline(col2h, center3 + 8, line1 - 2)
  instructions:triangle(col2h, line1 - 2, 90, 3)

  return instructions
end)()

function PitchSubView:addDrawing()
  local drawing = app.Drawing(0, 0, 128, 64)
  drawing:add(overlay)
  self.graphic:addChild(drawing)

  local two = app.Label("2", 12)
  two:fitToText(0)
  two:setCenter(col2h - 2, app.GRID5_CENTER3)
  self.graphic:addChild(two)

  local v = app.Label("v", 10)
  v:fitToText(0)
  v:setCenter(col2h + 3, app.GRID5_CENTER3 + 5)
  self.graphic:addChild(v)
end

function PitchSubView:init(args)
  Base.init(self, args)

  self:addDrawing()

  SubViewChain {
    parent   = self,
    position = 1,
    name     = "empty",
    branch   = args.branch,
    column   = app.BUTTON1_CENTER - 20,
    row      = app.GRID5_LINE4
  }

  SubViewReadout {
    parent        = self,
    position      = 2,
    name          = "tune",
    parameter     = args.tune,
    encoderMap    = Encoder.getMap("cents"),
    units         = app.unitCents,
    precision     = 0,
    editMessage   = "Pitch offset.",
    commitMessage = "Updated pitch offset.",
    column        = app.BUTTON2_CENTER,
    row           = app.GRID5_CENTER4
  }
end

function PitchSubView:onFocused()
  self:setFocusedPosition(2)
  Base.onFocused(self)
end

return PitchSubView