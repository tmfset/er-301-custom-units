local app = app
local lojik = require "lojik.liblojik"
local Drawings = require "Drawings"
local Utils = require "Utils"
local Class = require "Base.Class"
local ViewControl = require "Unit.ViewControl"

local ply = app.SECTION_PLY

local RegisterView = Class {
  type = "Display",
  canEdit = false,
  canMove = true
}
RegisterView:include(ViewControl)

function RegisterView:init(args)
  ViewControl.init(self)
  self:setClassName("lojik.RegisterView")

  local button = args.button or app.logError("%s.init: button is missing.", self)
  self:setInstanceName(button)

  local register = args.register or app.logError("%s.init: register is missing.", self)

  local graphic = lojik.RegisterMainView(0, 0, ply, 64)
  self.graphic = graphic

  self:setControlGraphic(self.graphic)
  self:addSpotDescriptor {
    center = 0.5 * ply
  }

  graphic:follow(register)
end

return RegisterView