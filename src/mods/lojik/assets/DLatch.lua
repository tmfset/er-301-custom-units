local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local UnitShared = require "shared.UnitShared"

local DLatch = Class {}
DLatch:include(Unit)
DLatch:include(UnitShared)

function DLatch:init(args)
  args.title = "DLatch"
  args.mnemonic = "DLx"
  Unit.init(self, args)
end

function DLatch:onLoadGraph(channelCount)
  local clock = self:addComparatorControl("clock", app.COMPARATOR_GATE)
  local reset = self:addComparatorControl("reset",  app.COMPARATOR_GATE)

  local op = self:addObject("op", lojik.DLatch())
  connect(self,  "In1", op, "In")
  connect(clock, "Out", op, "Clock")
  connect(reset, "Out", op, "Reset")

  for i = 1, channelCount do
    connect(op, "Out", self, "Out"..i)
  end
end

function DLatch:onLoadViews()
  return {
    clock = self:gateView("clock", "Clock"),
    reset = self:gateView("reset", "Reset")
  }, {
    expanded  = { "clock", "reset" },
    collapsed = {}
  }
end

return DLatch
