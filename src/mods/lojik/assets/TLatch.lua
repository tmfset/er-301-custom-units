local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local UnitShared = require "shared.UnitShared"

local TLatch = Class {}
TLatch:include(Unit)
TLatch:include(UnitShared)

function TLatch:init(args)
  args.title = "TLatch"
  args.mnemonic = "TLx"
  Unit.init(self, args)
end

function TLatch:onLoadGraph(channelCount)
  local duration = self:addGainBiasControl("duration")
  local reset    = self:addComparatorControl("reset",  app.COMPARATOR_GATE)

  local op = self:addObject("op", lojik.TLatch())
  connect(self,     "In1", op, "In")
  connect(duration, "Out", op, "Duration")
  connect(reset,    "Out", op, "Reset")

  for i = 1, channelCount do
    connect(op, "Out", self, "Out"..i)
  end
end

function TLatch:onShowMenu(objects)
  return {
    sensitivity = self.senseOptionControl(objects.op)
  }, { "sensitivity" }
end

function TLatch:onLoadViews()
  return {
    time = GainBias {
      button      = "time",
      branch      = self.branches.duration,
      description = "Duration",
      gainbias    = self.objects.duration,
      range       = self.objects.durationRange,
      biasMap     = self.linMap(0, 10, 0.1, 0.01, 0.001, 0.001),
      biasUnits   = app.unitSecs,
      initialBias = 0.1
    },
    reset = self:gateView("reset", "Reset")
  }, {
    expanded  = { "time", "reset" },
    collapsed = {}
  }
end

return TLatch
