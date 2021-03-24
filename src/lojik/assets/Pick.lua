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
  local alt = self:addGainBiasControl("alt")
  local pick  = self:addComparatorControl("pick", app.COMPARATOR_GATE)

  for i = 1, channelCount do
    local op = self:pick(pick, self, alt, "op"..i, "In"..i)
    connect(op, "Out", self, "Out"..i)
  end
end

function Pick:onLoadViews()
  return {
    alt   = GainBias {
      button        = "alt",
      description   = "Alternate",
      branch        = self.branches.alt,
      gainbias      = self.objects.alt,
      range         = self.objects.altRange,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialGain   = 1
    },
    pick  = self:gateView("pick", "Pick")
  }, {
    expanded  = { "alt", "pick" },
    collapsed = {}
  }
end

return Pick
