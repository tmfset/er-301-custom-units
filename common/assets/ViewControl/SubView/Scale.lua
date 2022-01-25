local app = app
local Class = require "Base.Class"

local Base      = require "common.assets.ViewControl.SubView"
local Chain     = require "common.assets.ViewControl.SubControl.Chain"
local Readout   = require "common.assets.ViewControl.SubControl.Readout"
local ScaleList = require "common.assets.ViewControl.SubControl.ScaleList"

local Scale = Class {}
Scale:include(Base)

function Scale:init(args)
  Base.init(self, args)

  Chain {
    parent   = self,
    position = 1,
    name     = "empty",
    branch   = args.branch,
    column   = app.BUTTON1_CENTER - 20,
    row      = app.GRID5_LINE4
  }

  Readout {
    parent        = self,
    position      = 2,
    name          = "gain",
    parameter     = args.gainBias:getParameter("Gain"),
    dialMap       = args.gainDialMap,
    units         = args.units,
    precision     = args.precision,
    editMessage   = string.format("'%s' modulation gain.", self.name),
    commitMessage = string.format("'%s' gain updated.", self.name),
    column        = app.BUTTON2_CENTER,
    row           = app.GRID5_CENTER4
  }

  ScaleList {
    parent   = self,
    position = 3,
    name     = "scale",
    source   = args.scaleSource,
    column   = app.BUTTON3_CENTER,
    row      = app.GRID5_CENTER4
  }
end

return Scale