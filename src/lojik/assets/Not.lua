local Class = require "Base.Class"
local Unit = require "Unit"
local Common = require "lojik.Common"

local Not = Class {}
Not:include(Unit)
Not:include(Common)

function Not:init(args)
  args.title = "Not"
  args.mnemonic = "!"
  Unit.init(self, args)
end

function Not:onLoadGraph(channelCount)
  for i = 1, channelCount do
    local op = self:lNot(self, "op"..i, "In"..i)
    connect(op, "Out", self, "Out"..i)
  end
end

function Not:onLoadViews()
  return
end

return Not
