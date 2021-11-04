local app   = app
local Class = require "Base.Class"

local Base      = require "polygon.SubViewPitch"
local SubToggle = require "polygon.SubToggle"

local SubViewPitchTrack = Class {}
SubViewPitchTrack:include(Base)

function SubViewPitchTrack:init(args)
  Base.init(self, args)

  SubToggle {
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

return SubViewPitchTrack