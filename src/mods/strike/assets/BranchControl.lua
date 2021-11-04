local Class = require "Base.Class"
local ViewControl = require "Unit.ViewControl"

local BranchControl = Class {}
BranchControl:include(ViewControl)

function BranchControl:init(args)
  local name = args.name or app.logError("%s.init: name is missing.", self)
  local branch = args.branch or app.logError("%s.init: branch is missing.", self)
  ViewControl.init(self, name)
  self.branch = branch
end

return BranchControl