local app = app
local polygon = require "polygon.libpolygon"
local Drawings = require "Drawings"
local Utils = require "Utils"
local Class = require "Base.Class"
local PagedViewControl = require "polygon.PagedViewControl"

local ply = app.SECTION_PLY

local RoundRobin = Class {
  type    = "RoundRobin",
  canEdit = false,
  canMove = true
}
RoundRobin:include(PagedViewControl)

function RoundRobin:init(args)
  PagedViewControl.init(self, args)
  self:setClassName("polygon.RoundRobin")

  self.polygon = args.polygon or app.logError("%s.init: missing polygon instance.", self)

  self.graphic = polygon.RoundRobinView(self.polygon, 0, 0, ply, 64)

  self:setControlGraphic(self.graphic)
  self:addSpotDescriptor {
    center = 0.5 * ply
  }
end

function RoundRobin:subReleased(i, shifted)
  self.polygon:releaseManualGates()
  return PagedViewControl.subReleased(self, i, shifted)
end

return RoundRobin