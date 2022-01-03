local app   = app
local Class = require "Base.Class"
local Base  = require "common.assets.ViewControl.SubControl"

local Button = Class {}
Button:include(Base)

function Button:init(args)
  Base.init(self, args)
  self.onPress   = args.onPress or self.onPress
  self.onRelease = args.onRelease or self.onRelease
end

return Button
