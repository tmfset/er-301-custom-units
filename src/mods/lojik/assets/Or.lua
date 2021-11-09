local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local UnitShared = require "shared.UnitShared"

local Or = Class {}
Or:include(Unit)
Or:include(UnitShared)

function Or:init(args)
  args.title = "Or"
  args.mnemonic = "||"
  Unit.init(self, args)
end

function Or:onLoadGraph(channelCount)
  local gate = self:addComparatorControl("gate", app.COMPARATOR_GATE)

  local op = self:addObject("op", lojik.Or())
  connect(self, "In1", op, "In")
  connect(gate, "Out", op, "Gate")

  for i = 1, channelCount do
    connect(op, "Out", self, "Out"..i)
  end
end

function Or:onShowMenu(objects)
  return {
    sensitivity = self.senseOptionControl(objects.op)
  }, { "sensitivity" }
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
