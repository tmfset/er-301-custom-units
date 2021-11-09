local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Encoder = require "Encoder"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local UnitShared = require "common.assets.UnitShared"

local Pick = Class {}
Pick:include(Unit)
Pick:include(UnitShared)

function Pick:init(args)
  args.title = "Pick"
  args.mnemonic = "P"
  Unit.init(self, args)
end

function Pick:onLoadGraph(channelCount)
  local alt = self:addGainBiasControl("alt")
  local pick  = self:addComparatorControl("pick", app.COMPARATOR_GATE)

  local op = self:addObject("op", lojik.Pick())
  connect(self, "In1", op, "In")
  connect(alt,  "Out", op, "Alt")
  connect(pick, "Out", op, "Pick")

  for i = 1, channelCount do
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
