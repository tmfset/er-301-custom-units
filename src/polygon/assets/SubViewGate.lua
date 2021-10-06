local app   = app
local Class = require "Base.Class"

local Base       = require "polygon.SubView"
local SubReadout = require "polygon.SubReadout"
local SubChain   = require "polygon.SubChain"
local SubButton  = require "polygon.SubButton"

local SubGateView = Class {}
SubGateView:include(Base)

local overlay = (function ()
  local instructions = app.DrawingInstructions()

  local line1 = app.GRID5_LINE1
  local line4 = app.GRID5_LINE4

  local center3 = app.GRID5_CENTER3

  local col1 = app.BUTTON1_CENTER
  local col2 = app.BUTTON2_CENTER
  local col3 = app.BUTTON3_CENTER

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

function SubGateView:addDrawing()
  local drawing = app.Drawing(0, 0, 128, 64)
  drawing:add(overlay)
  self.graphic:addChild(drawing)

  local label = app.Label("or", 10)
  label:fitToText(0)
  label:setCenter(app.BUTTON3_CENTER + 1, app.GRID5_CENTER3 + 1)
  self.graphic:addChild(label)
end

function SubGateView:init(args)
  Base.init(self, args)

  self:addDrawing()

  SubChain {
    parent   = self,
    position = 1,
    name     = "empty",
    branch   = args.branch,
    column   = app.BUTTON1_CENTER - 20,
    row      = app.GRID5_LINE4
  }

  SubReadout {
    parent        = self,
    position      = 2,
    name          = "thresh",
    parameter     = args.threshold,
    editMessage   = "Gate detection threshold.",
    commitMessage = "Updated gate detection threshold.",
    column        = app.BUTTON2_CENTER,
    row           = app.GRID5_CENTER4
  }

  SubButton {
    parent    = self,
    position  = 3,
    name      = "fire",
    onPress   = args.onPressFire,
    onRelease = args.onReleaseFire
  }
end

return SubGateView