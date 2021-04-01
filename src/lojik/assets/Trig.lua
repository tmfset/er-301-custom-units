local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local Common = require "lojik.Common"

local Trig = Class {}
Trig:include(Unit)
Trig:include(Common)

function Trig:init(args)
  args.title = "Trig"
  args.mnemonic = "T"
  Unit.init(self, args)
end

function Trig:onShowMenu(objects)
  return {
    sensitivity = self.senseOptionControl(objects.op)
  }, { "sensitivity" }
end

function Trig:onLoadGraph(channelCount)
  local op = self:addObject("op", lojik.Trig())
  connect(self, "In1", op, "In")

  for i = 1, channelCount do
    connect(op, "Out", self, "Out"..i)
  end
end

function Trig:onLoadViews()
  return
end

return Trig
