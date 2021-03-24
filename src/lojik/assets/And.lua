local app = app
local Class = require "Base.Class"
local Unit = require "Unit"
local Common = require "lojik.Common"

local And = Class {}
And:include(Unit)
And:include(Common)

function And:init(args)
  args.title = "And"
  args.mnemonic = "&&"
  Unit.init(self, args)
end

function And:onLoadGraph(channelCount)
  local gate = self:addComparatorControl("gate", app.COMPARATOR_GATE)

  for i = 1, channelCount do
    local op = self:lAnd(self, gate, "op"..i, "In"..i)
    connect(op, "Out", self, "Out"..i)
  end
end

function And:onLoadViews()
  return {
    gate = self:gateView("gate", "Gate")
  }, {
    expanded  = { "gate" },
    collapsed = {}
  }
end

return And
