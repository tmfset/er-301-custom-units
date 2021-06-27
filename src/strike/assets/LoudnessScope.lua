local app = app
local strike = require "strike.libstrike"
local Env = require "Env"
local Class = require "Base.Class"
local ViewControl = require "Unit.ViewControl"
local Encoder = require "Encoder"
local ply = app.SECTION_PLY

local line1 = app.GRID4_LINE1
local line2 = app.GRID4_LINE2
local line3 = app.GRID4_LINE3
local line4 = app.GRID4_LINE4
local col1 = app.BUTTON1_CENTER
local col2 = app.BUTTON2_CENTER
local col3 = app.BUTTON3_CENTER

local LoudnessScope = Class {}
LoudnessScope:include(ViewControl)

function LoudnessScope:init(args)
  ViewControl.init(self)
  self:setClassName("strike.LoudnessScope")
  local width = args.width or ply
  local loudness = args.loudness or app.logError("%s.init: loudness is missing.", self)
  local reduction = args.reduction or app.logError("%s.init: reduction is missing.", self)

  local graphic = app.Graphic(0, 0, width, 64)
  self:setMainCursorController(graphic)
  self:setControlGraphic(graphic)
  self.subGraphic = app.Graphic(0, 0, 128, 64)

  -- add spots
  for i = 1, (width // ply) do
    self:addSpotDescriptor{
      center = (i - 0.5) * ply
    }
  end
  self.verticalDivider = width

  self.scope = strike.LoudnessScope(0, 0, width, 64)
  self.scope:watchOutlet(loudness)
  graphic:addChild(self.scope)
end

function LoudnessScope:spotReleased(spot, shifted)
  return true
end

return LoudnessScope
