local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Base = require "Unit.ViewControl.EncoderControl"
local Encoder = require "Encoder"
local UnitShared = require "common.assets.UnitShared"

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

local RegisterCircle = Class {
  type    = "RegisterCircle",
  canEdit = false,
  canMove = true
}
RegisterCircle:include(Base)
RegisterCircle:include(UnitShared)

function RegisterCircle:init(args)
  local name     = args.name or app.logError("%s.init: name is missing.", self)
  local register = args.register or app.logError("%s.init: register is missing.", self)

  Base.init(self, name)
  self:setClassName("RegisterCircle")

  local width = args.width or (2 * ply)

  local graphic = app.Graphic(0, 0, width, 64)
  local splitTop = app.SCREEN_HEIGHT * 0.666
  local splitBottom = app.SCREEN_HEIGHT - splitTop
  graphic:addChild(lojik.RegisterCircle(register, 0, splitBottom, width, splitTop))
  graphic:addChild(lojik.RegisterChart(register, 0, 0, width, splitBottom))

  self.graphic = graphic
  self:setControlGraphic(self.graphic)
  for i = 1, (width // ply) do
    self:addSpotDescriptor {
      center = (i - 0.5) * ply
    }
  end
end

return RegisterCircle