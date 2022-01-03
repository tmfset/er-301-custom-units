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

local RegisterView = Class {
  type    = "RegisterView",
  canEdit = false,
  canMove = true
}
RegisterView:include(Base)
RegisterView:include(UnitShared)

function RegisterView:init(args)
  self:setClassName("RegisterView")

  local name     = args.name or app.logError("%s.init: name is missing.", self)
  self.register = args.register or app.logError("%s.init: register is missing.", self)
  Base.init(self, name)

  local width = args.width or (2 * ply)
  local graphic = app.Graphic(0, 0, width, 64)
  graphic:addChild(lojik.RegisterMainView(self.register, 0, 0, width, app.SCREEN_HEIGHT))

  self.graphic = graphic
  self:setControlGraphic(self.graphic)
  for i = 1, (width // ply) do
    self:addSpotDescriptor {
      center = (i - 0.5) * ply
    }
  end
end

function RegisterView:getFloatingMenuItems()
  return {
    "offset",
    "shift",
    "randomize"
  }
end

function RegisterView:onFloatingMenuSelection(choice)
  if choice == "randomize" then
    self.register:triggerRandomizeWindow();
  end
end

return RegisterView