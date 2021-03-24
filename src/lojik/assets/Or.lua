local app = app
local Class = require "Base.Class"
local Unit = require "Unit"
local Common = require "lojik.Common"

local Or = Class {}
Or:include(Unit)
Or:include(Common)

function Or:init(args)
  args.title = "Or"
  args.mnemonic = "||"
  Unit.init(self, args)
end

function Or:onLoadGraph(channelCount)
  local gate = self:addComparatorControl("gate", app.COMPARATOR_GATE)

  for i = 1, channelCount do
    local op = self:lOr(self, gate, "op"..i, "In"..i)
    connect(op, "Out", self, "Out"..i)
  end
end

function Or:onLoadViews()
  return {
    gate = self:gateView("gate", "Gate")
  }, {
    expanded  = { "gate" },
    collapsed = {}
  }
end

return Or
