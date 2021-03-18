local app = app
local Class = require "Base.Class"
local Encoder = require "Encoder"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Common = require "lojik.Common"

local Pick = Class {}
Pick:include(Unit)
Pick:include(Common)

function Pick:init(args)
  args.title = "Pick"
  args.mnemonic = "P"
  Unit.init(self, args)
end

function Pick:onLoadGraph(channelCount)
  local left  = self:addGainBiasControl("left")
  local right = self:addGainBiasControl("right")
  local pick  = self:addComparatorControl("pick", app.COMPARATOR_GATE)

  for i = 1, channelCount do
    local op = self:pick(pick, left, right, "op"..i)
    connect(op, "Out", self, "Out"..i)
  end
end

function Pick:onLoadViews()
  return {
    left   = GainBias {
      button        = "left",
      description   = "Left",
      branch        = self.branches.left,
      gainbias      = self.objects.left,
      range         = self.objects.leftRange,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialGain   = 1
    },
    right   = GainBias {
      button        = "right",
      description   = "Right",
      branch        = self.branches.right,
      gainbias      = self.objects.right,
      range         = self.objects.rightRange,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialGain   = 1
    },
    pick  = self:gateView("pick", "Pick")
  }, {
    expanded  = { "left", "right", "pick" },
    collapsed = {}
  }
end

return Pick
