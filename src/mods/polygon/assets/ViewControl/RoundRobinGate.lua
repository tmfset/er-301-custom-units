local app     = app
local polygon = require "polygon.libpolygon"
local Class   = require "Base.Class"

local Base = require "common.assets.ViewControl.Split.Paged"
local Gate = require "common.assets.ViewControl.Sub.View.Gate"

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
  self.branch = args.branch or app.logError("%s.init: missing branch.", self)

  local threshold = self.polygon:getParameter("Gate Threshold")
  local onReleaseFire = function () self.polygon:releaseManualGates() end

  self:addSubView(Gate {
    name          = "Round Robin",
    branch        = self.branch,
    threshold     = threshold,
    onPressFire   = function () self.polygon:markManualGate(0) end,
    onReleaseFire = onReleaseFire
  })

  for i, voice in ipairs(args.voices) do
    self:addSubView(Gate {
      name          = "Voice "..i,
      branch        = voice.gateBranch,
      threshold     = threshold,
      onPressFire   = function () self.polygon:markManualGate(i) end,
      onReleaseFire = onReleaseFire
    })
  end

  self.graphic = polygon.RoundRobinGateView(self.polygon, 0, 0, ply, 64)
  self:setControlGraphic(self.graphic)
  self:addSpotDescriptor {
    center = 0.5 * ply
  }
end

function RoundRobinGate:updatePageIndex(pageIndex, propogate)
  self.graphic:setCursorSelection(pageIndex - 1)
  Base.updatePageIndex(self, pageIndex, propogate)
end

return RoundRobinGate
