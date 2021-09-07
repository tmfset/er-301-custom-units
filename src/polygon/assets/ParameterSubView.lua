local app   = app
local Class = require "Base.Class"
local Base           = require "polygon.SubView"
local SubViewReadout = require "polygon.SubViewReadout"

local ParameterSubView = Class {}
ParameterSubView:include(Base)

function ParameterSubView:init(args)
  Base.init(self, args)

  SubViewReadout {
    parent        = self,
    position      = 1,
    name          = args.param1.name,
    parameter     = args.param1.parameter,
    editMessage   = args.param1.editMessage,
    commitMessage = args.param1.commitMessage,
    column        = app.BUTTON1_CENTER,
    row           = app.GRID5_CENTER4
  }

  SubViewReadout {
    parent        = self,
    position      = 2,
    name          = args.param2.name,
    parameter     = args.param2.parameter,
    editMessage   = args.param2.editMessage,
    commitMessage = args.param2.commitMessage,
    column        = app.BUTTON2_CENTER,
    row           = app.GRID5_CENTER4
  }

  SubViewReadout {
    parent        = self,
    position      = 3,
    name          = args.param3.name,
    parameter     = args.param3.parameter,
    editMessage   = args.param3.editMessage,
    commitMessage = args.param3.commitMessage,
    column        = app.BUTTON3_CENTER,
    row           = app.GRID5_CENTER4
  }
end

return ParameterSubView