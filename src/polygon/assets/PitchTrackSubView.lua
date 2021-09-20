local app   = app
local Class = require "Base.Class"

local Base          = require "polygon.PitchSubView"
local SubViewToggle = require "polygon.SubViewToggle"

local PitchTrackSubView = Class {}
PitchTrackSubView:include(Base)

function PitchTrackSubView:init(args)
  Base.init(self, args)

  SubViewToggle {
    parent    = self,
    position  = 3,
    name      = "track",
    option    = args.option,
    on        = 1,
    off       = 2,
    column    = app.BUTTON3_CENTER,
    row       = app.GRID5_CENTER3
  }
end

return PitchTrackSubView