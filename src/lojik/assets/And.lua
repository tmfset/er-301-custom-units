local app = app
local Class = require "Base.Class"
local Unit = require "Unit"
local Gate = require "Unit.ViewControl.Gate"
local Common = require "lojik.Common"

local And = Class {}
And:include(Common)

function And:init(args)
  args.title = "And"
  args.mnemonic = "&&"
  Unit.init(self, args)
end

function And:onLoadGraph(channelCount)
  local left  = self:addComparatorControl("left",  app.COMPARATOR_GATE)
  local right = self:addComparatorControl("right", app.COMPARATOR_GATE)

  for i = 1, channelCount do
    local op = self:lAnd(left, right, "op"..i)
    connect(op, "Out", self, "Out"..i)
  end
end

function And:onLoadViews(objects, branches)
  return {
    left = Gate {
      button      = "left",
      description = "Left",
      branch      = branches.left,
      comparator  = objects.left
    },
    right = Gate {
      button      = "right",
      description = "Right",
      branch      = branches.right,
      comparator  = objects.right
    }
  }, {
    expanded  = { "left", "right" },
    collapsed = {}
  }
end

return And
