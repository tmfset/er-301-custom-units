local app = app
local polygon = require "polygon.libpolygon"
local Drawings = require "Drawings"
local Utils = require "Utils"
local Class = require "Base.Class"
local ViewControl = require "Unit.ViewControl"

local ply = app.SECTION_PLY

local RoundRobin = Class {
  type    = "RoundRobin",
  canEdit = false,
  canMove = true
}
RoundRobin:include(ViewControl)

function RoundRobin:init(args)
  ViewControl.init(self)
  self:setClassName("polygon.RoundRobin")

  self.graphic = polygon.RoundRobinView(args.polygon, 0, 0, ply, 64)

  self:setControlGraphic(self.graphic)
  self:addSpotDescriptor {
    center = 0.5 * ply
  }
end

return RoundRobin