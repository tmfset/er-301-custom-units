local app     = app
local Class   = require "Base.Class"
local Encoder = require "Encoder"
local Settings = require "Settings"

local Base              = require "polygon.SplitViewControl"
local SubViewPitchTrack = require "polygon.SubViewPitchTrack"

local TrackablePitch = Class {
  type = "TrackablePitch",
  canEdit = false,
  canMove = false
}
TrackablePitch:include(Base)

function TrackablePitch:init(args)
  Base.init(self, args)

  local offset = args.offset or app.logError("%s.init: offset is missing.", self)
  local range  = args.range or app.logError("%s.init: range is missing.", self)
  self.branch = args.branch or app.logError("%s.init: missing branch.", self)

  local faderParam   = offset:getParameter("Offset") or offset:getParameter("Bias")
  local readoutParam
  if Settings.get("unitControlReadoutSource") == "actual" then
    readoutParam = range:getParameter("Center") or faderParam
  else
    readoutParam = faderParam
  end

  self:addSubView(SubViewPitchTrack {
    name   = args.name,
    branch = self.branch,
    tune   = faderParam,
    option = args.track
  })

  self.graphic = app.Fader(0, 0, app.SECTION_PLY, 64)
  self.graphic:setTargetParameter(faderParam)
  self.graphic:setValueParameter(faderParam)
  self.graphic:setControlParameter(readoutParam)
  self.graphic:setRangeObject(range)
  self.graphic:setLabel(args.name)
  self.graphic:setAttributes(app.unitCents, Encoder.getMap("cents"))
  self.graphic:setPrecision(0)

  self:setMainCursorController(self.graphic)
  self:setControlGraphic(self.graphic)
  self:addSpotDescriptor {
    center = 0.5 * app.SECTION_PLY
  }
end

return TrackablePitch