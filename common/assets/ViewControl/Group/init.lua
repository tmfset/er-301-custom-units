local app   = app
local Class = require "Base.Class"
local Base  = require "Unit.ViewControl.EncoderControl"

local Group = Class {}
Group:include(Base)

function Group:init(args)
  Base.init(self, args.name)
end

return Group
