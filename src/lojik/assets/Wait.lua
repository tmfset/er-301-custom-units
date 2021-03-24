local app = app
local Class = require "Base.Class"
local Encoder = require "Encoder"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Common = require "lojik.Common"

local Wait = Class {}
Wait:include(Unit)
Wait:include(Common)

function Wait:init(args)
  args.title = "Wait"
  args.mnemonic = "W"
  self.max = 64
  Unit.init(self, args)
end

function Wait:onLoadGraph(channelCount)
  local wait  = self:addGainBiasControl("wait")
  local clock = self:addComparatorControl("clock", app.COMPARATOR_TRIGGER_ON_RISE)
  local reset = self:addComparatorControl("reset", app.COMPARATOR_GATE)

  for i = 1, channelCount do
    local op = self:wait(self, wait, clock, reset, "op"..i, "In"..i)
    connect(op, "Out", self, "Out"..i)
  end
end

function Wait:onLoadViews()
  return {
    wait  = GainBias {
      button        = "wait",
      description   = "Wait",
      branch        = self.branches.wait,
      gainbias      = self.objects.wait,
      range         = self.objects.waitRange,
      gainMap       = self.intMap(-self.max, self.max),
      biasMap       = self.intMap(0, self.max),
      biasPrecision = 0,
      initialBias   = 4
    },
    clock  = self:gateView("clock", "Clock"),
    reset = self:gateView("reset", "Reset")
  }, {
    expanded  = { "wait", "clock", "reset" },
    collapsed = {}
  }
end

return Wait
