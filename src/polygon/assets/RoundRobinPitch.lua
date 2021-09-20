local app     = app
local polygon = require "polygon.libpolygon"
local Class   = require "Base.Class"

local Base              = require "polygon.PagedViewControl"
local PitchSubView      = require "polygon.PitchSubView"
local PitchTrackSubView = require "polygon.PitchTrackSubView"

local ply = app.SECTION_PLY

local RoundRobinPitch = Class {
  type    = "RoundRobinPitch",
  canEdit = false,
  canMove = true
}
RoundRobinPitch:include(Base)

function RoundRobinPitch:init(args)
  Base.init(self, args)
  self.polygon = args.polygon or app.logError("%s.init: missing polygon instance.", self)
  local biasMap = args.biasMap or app.logError("%s.init: missing bias map.", self)

  self:addSubView(PitchTrackSubView {
    name   = "Round Robin",
    branch = args.branch,
    tune   = args.tune,
    option = self.polygon:getOption("RR V/Oct Track")
  })

  for i, voice in ipairs(args.voices) do
    self:addSubView(PitchSubView {
      name   = "Voice "..i,
      branch = voice.pitchBranch,
      tune   = voice.pitchOffset
    })
  end

  self.graphic = polygon.RoundRobinPitchView(self.polygon, 0, 0, ply, 64)
  self.graphic:setScale(biasMap)
  self:setControlGraphic(self.graphic)
  self:addSpotDescriptor {
    center = 0.5 * ply
  }
end

return RoundRobinPitch