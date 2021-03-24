local app = app
local Class = require "Base.Class"
local Unit = require "Unit"
local Common = require "lojik.Common"

local Latch = Class {}
Latch:include(Unit)
Latch:include(Common)

function Latch:init(args)
  args.title = "Latch"
  args.mnemonic = "Lx"
  Unit.init(self, args)
end

function Latch:onLoadGraph(channelCount)
  local reset = self:addComparatorControl("reset", app.COMPARATOR_TRIGGER_ON_RISE)

  for i = 1, channelCount do
    local latch = self:latch(self, reset, "latch"..i, "In"..i)
    connect(latch, "Out", self, "Out"..i)
  end
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
