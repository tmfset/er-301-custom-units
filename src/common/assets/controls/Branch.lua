local Class = require "Base.Class"
local ViewControl = require "Unit.ViewControl"

local Branch = Class {}
Branch:include(ViewControl)

function Branch:init(args)
  local name = args.name or app.logError("%s.init: name is missing.", self)
  local branch = args.branch or app.logError("%s.init: branch is missing.", self)
  ViewControl.init(self, name)
  self.branch = branch
end

return Branch