local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local UnitShared = require "shared.UnitShared"

local Not = Class {}
Not:include(Unit)
Not:include(UnitShared)

function Not:init(args)
  args.title = "Not"
  args.mnemonic = "!"
  Unit.init(self, args)
end

function Not:onLoadGraph(channelCount)
  local op = self:addObject("op", lojik.Not())
  connect(self, "In1", op, "In")

  for i = 1, channelCount do
    connect(op, "Out", self, "Out"..i)
  end
end

function Not:onShowMenu(objects)
  return {
    sensitivity = self.senseOptionControl(objects.op)
  }, { "sensitivity" }
end

function Not:onLoadViews()
  return
end

return Not
