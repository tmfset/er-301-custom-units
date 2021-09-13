local app     = app
local polygon = require "polygon.libpolygon"

local Drawings = require "Drawings"
local Utils    = require "Utils"
local Class    = require "Base.Class"

local Base             = require "polygon.PagedViewControl"
local GateSubView      = require "polygon.GateSubView"
local ParameterSubView = require "polygon.ParameterSubView"

local ply = app.SECTION_PLY

local RoundRobinGate = Class {
  type    = "RoundRobinGate",
  canEdit = false,
  canMove = true
}
RoundRobinGate:include(Base)

function RoundRobinGate:init(args)
  Base.init(self, args)
  self.polygon = args.polygon or app.logError("%s.init: missing polygon instance.", self)

  local threshold = self.polygon:getParameter("Gate Threshold")
  local onReleaseFire = function () self.polygon:releaseManualGates() end

  self:addSubView(GateSubView {
    name          = "Round Robin",
    branch        = args.branch,
    threshold     = threshold,
    onPressFire   = function () self.polygon:markRRManualGate() end,
    onReleaseFire = onReleaseFire
  })

  -- self:addSubView(ParameterSubView {
  --   name        = "Settings",
  --   param1 = {
  --     name          = "count",
  --     parameter     = self.polygon:getParameter("RR Count"),
  --     editMessage   = "",
  --     commitMessage = ""
  --   },
  --   param2 = {
  --     name          = "stride",
  --     parameter     = self.polygon:getParameter("RR Stride"),
  --     editMessage   = "",
  --     commitMessage = ""
  --   },
  --   param3 = {
  --     name          = "total",
  --     parameter     = self.polygon:getParameter("RR Total"),
  --     editMessage   = "",
  --     commitMessage = ""
  --   }
  -- })

  for i, branch in ipairs(args.gates) do
    self:addSubView(GateSubView {
      name          = "Voice "..i,
      branch        = branch,
      threshold     = threshold,
      onPressFire   = function () self.polygon:markManualGate(i - 1) end,
      onReleaseFire = onReleaseFire
    })
  end

  self.graphic = polygon.RoundRobinView(self.polygon, 0, 0, ply, 64)
  self:setControlGraphic(self.graphic)
  self:addSpotDescriptor {
    center = 0.5 * ply
  }
end

return RoundRobinGate