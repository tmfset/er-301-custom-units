local app = app
local Class = require "Base.Class"
local Unit = require "Unit"
local Common = require "lojik.Common"

local Latch = Class {}
Latch:include(Unit)
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

function Latch:onLoadViews()
  return {
    set   = self:gateView("set", "Set"),
    reset = self:gateView("reset", "Reset")
  }, {
    expanded  = { "set", "reset" },
    collapsed = {}
  }
end

return Latch
