local app = app
local Class = require "Base.Class"
local Unit = require "Unit"
local Gate = require "Unit.ViewControl.Gate"
local Common = require "lojik.Common"

local DLatch = Class {}
DLatch:include(Common)

function DLatch:init(args)
  args.title = "DLatch"
  args.mnemonic = "DL"
  Unit.init(self, args)
end

function DLatch:onLoadGraph(channelCount)
  local clock = self:addComparatorControl("clock", app.COMPARATOR_TRIGGER_ON_RISE)
  local reset = self:addComparatorControl("reset",  app.COMPARATOR_TRIGGER_ON_RISE)

  for i = 1, channelCount do
    local latch = self:dLatch(self, clock, reset, "latch"..i, "In"..i)
    connect(latch, "Out", self, "Out"..i)
  end
end

function DLatch:onLoadViews(objects, branches)
  return {
    clock = Gate {
      button      = "clock",
      description = "Clock",
      branch      = branches.clock,
      comparator  = objects.clock
    },
    reset = Gate {
      button      = "reset",
      description = "Reset",
      branch      = branches.reset,
      comparator  = objects.reset
    }
  }, {
    expanded  = { "clock", "reset" },
    collapsed = {}
  }
end

return DLatch
