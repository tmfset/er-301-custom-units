local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local UnitShared = require "common.assets.UnitShared"

local Latch = Class {}
Latch:include(Unit)
Latch:include(UnitShared)

function Latch:init(args)
  args.title = "Latch"
  args.mnemonic = "Lx"
  Unit.init(self, args)
end

function Latch:onLoadGraph(channelCount)
  local reset = self:addComparatorControl("reset", app.COMPARATOR_TRIGGER_ON_RISE)

  local op = self:addObject("op", lojik.Latch())
  connect(self,  "In1", op, "In")
  connect(reset, "Out", op, "Reset")

  for i = 1, channelCount do
    connect(op, "Out", self, "Out"..i)
  end
end

function Latch:onShowMenu(objects)
  return {
    sensitivity = self.senseOptionControl(objects.op)
  }, { "sensitivity" }
end

function Latch:onLoadViews()
  return {
    reset = self:gateView("reset", "Reset")
  }, {
    expanded  = { "reset" },
    collapsed = {}
  }
end

return Latch
