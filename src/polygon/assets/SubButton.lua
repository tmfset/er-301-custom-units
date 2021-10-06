local app = app
local Class = require "Base.Class"
local Base = require "polygon.SubControl"

local SubButton = Class {}
SubButton:include(Base)

function SubButton:init(args)
  Base.init(self, args)
  self.onPress   = args.onPress or self.onPress
  self.onRelease = args.onRelease or self.onRelease
end

return SubButton