local app = app
local Class = require "Base.Class"
local Unit = require "Unit"
local Gate = require "Unit.ViewControl.Gate"
local Common = require "lojik.Common"

local Latch = Class {}
Latch:include(Common)

function Latch:init(args)
  args.title = "Latch"
  args.mnemonic = "L"
  Unit.init(self, args)
end

function Latch:onLoadGraph(channelCount)
  local set   = self:addComparatorControl("set",   app.COMPARATOR_TRIGGER_ON_RISE)
  local reset = self:addComparatorControl("reset", app.COMPARATOR_TRIGGER_ON_RISE)

  for i = 1, channelCount do
    local latch = self:latch(set, reset, "latch"..i)
    connect(latch, "Out", self, "Out"..i)
  end
end

function Latch:onLoadViews(objects, branches)
  return {
    set = Gate {
      button      = "set",
      description = "Set",
      branch      = branches.set,
      comparator  = objects.set
    },
    reset = Gate {
      button      = "reset",
      description = "Reset",
      branch      = branches.reset,
      comparator  = objects.reset
    }
  }, {
    expanded  = { "set", "reset" },
    collapsed = {}
  }
end

return Latch
