local app   = app
local Class = require "Base.Class"

local Base    = require "common.assets.ViewControl.SubView"
local Readout = require "common.assets.ViewControl.SubControl.Readout"

local Parameter = Class {}
Parameter:include(Base)

function Parameter:init(args)
  Base.init(self, args)

  Readout {
    parent        = self,
    position      = 1,
    name          = args.param1.name,
    parameter     = args.param1.parameter,
    editMessage   = args.param1.editMessage,
    commitMessage = args.param1.commitMessage,
    column        = app.BUTTON1_CENTER,
    row           = app.GRID5_CENTER4
  }

  Readout {
    parent        = self,
    position      = 2,
    name          = args.param2.name,
    parameter     = args.param2.parameter,
    editMessage   = args.param2.editMessage,
    commitMessage = args.param2.commitMessage,
    column        = app.BUTTON2_CENTER,
    row           = app.GRID5_CENTER4
  }

  Readout {
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

return Parameter
