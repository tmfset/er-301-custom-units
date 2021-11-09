local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local UnitShared = require "common.assets.UnitShared"

local And = Class {}
And:include(Unit)
And:include(UnitShared)

function And:init(args)
  args.title = "And"
  args.mnemonic = "&&"
  Unit.init(self, args)
end

function And:onLoadGraph(channelCount)
  local gate = self:addComparatorControl("gate", app.COMPARATOR_GATE)

  local op = self:addObject("op", lojik.And())
  connect(self, "In1", op, "In")
  connect(gate, "Out", op, "Gate")

  for i = 1, channelCount do
    connect(op, "Out", self, "Out"..i)
  end
end

function And:onShowMenu(objects)
  return {
    sensitivity = self.senseOptionControl(objects.op)
  }, { "sensitivity" }
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
