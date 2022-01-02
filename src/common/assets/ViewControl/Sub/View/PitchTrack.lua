local app   = app
local Class = require "Base.Class"

local Base   = require "common.assets.ViewControl.Sub.View.Pitch"
local Toggle = require "common.assets.ViewControl.Sub.Control.Toggle"

local PitchTrack = Class {}
PitchTrack:include(Base)

function PitchTrack:init(args)
  Base.init(self, args)

  Toggle {
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

return PitchTrack
