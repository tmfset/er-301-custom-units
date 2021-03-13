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
  local left  = self:addComparatorControl("left",  app.COMPARATOR_GATE)
  local right = self:addComparatorControl("right", app.COMPARATOR_GATE)

  for i = 1, channelCount do
    local op = self:lOr(left, right, "op"..i)
    connect(op, "Out", self, "Out"..i)
  end
end

function Or:onLoadViews()
  return {
    left  = self:gateView("left", "Left"),
    right = self:gateView("right", "Right")
  }, {
    expanded  = { "left", "right" },
    collapsed = {}
  }
end

return Or
