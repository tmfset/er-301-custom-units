local app = app
local Class = require "Base.Class"
local Base = require "polygon.SubViewControl"

local SubViewButton = Class {}
SubViewButton:include(Base)

function SubViewButton:init(args)
  Base.init(self, args)
  self.onPress   = args.onPress or self.onPress
  self.onRelease = args.onRelease or self.onRelease
end

return SubViewButton