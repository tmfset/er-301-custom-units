local app   = app
local Class = require "Base.Class"
local Base  = require "common.assets.ViewControl.Spot"

local Single = Class {}
Single:include(Base)

function Single:init(args)
  Base.init(self, args)
  self.mainGraphic = app.Graphic(0, 0, app.SECTION_PLY, app.SCREEN_HEIGHT)
  self.subGraphic = app.Graphic(0, 0, app.SUB_SCREEN_WIDTH, app.SCREEN_HEIGHT)
end



return Single
