local Class = require "Base.Class"
local Unit = require "Unit"
local Common = require "lojik.Common"

local Trig = Class {}
Trig:include(Common)

function Trig:init(args)
  args.title = "Trig"
  args.mnemonic = "T"
  Unit.init(self, args)
end

function Trig:onLoadGraph(channelCount)
  for i = 1, channelCount do
    local op = self:trig(self, "op"..i, "In"..i)
    connect(op, "Out", self, "Out"..i)
  end
end

function Trig:onLoadViews()
  return
end

return Trig
