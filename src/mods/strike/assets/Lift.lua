local app = app
local strike = require "strike.libstrike"
local Class = require "Base.Class"
local Encoder = require "Encoder"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"
local UnitShared = require "shared.UnitShared"

local Lift = Class {}
Lift:include(Unit)
Lift:include(UnitShared)

function Lift:init(args)
  args.title = "Lift"
  args.mnemonic = "lpg"
  Unit.init(self, args)
end

function Lift:onLoadGraph(channelCount)
  local gate   = self:addComparatorControl("gate", app.COMPARATOR_GATE)
  local rise   = self:addParameterAdapterControl("rise")
  local fall   = self:addParameterAdapterControl("fall")
  local height = self:addParameterAdapterControl("height")

  local op = self:addObject("op", strike.Lift())
  tie(op, "Rise", rise, "Out")
  tie(op, "Fall", fall, "Out")
  tie(op, "Height", height, "Out")
  connect(gate, "Out", op, "Gate")

  for i = 1, channelCount do
    connect(self, "In"..i, op, "In"..i)
    connect(op, "Out"..i, self, "Out"..i)
  end

  self:addFreeBranch("env", self.objects.op, "Env")
end

function Lift:onLoadViews()
  return {
    env = self:branchView("env"),
    gate = Gate {
      button      = "gate",
      description = "Gate",
      branch      = self.branches.gate,
      comparator  = self.objects.gate
    },
    height = GainBias {
      button      = "height",
      description = "Height",
      branch      = self.branches.height,
      gainbias    = self.objects.height,
      range       = self.objects.height,
      biasMap     = Encoder.getMap("oscFreq"),
      biasUnits   = app.unitHertz,
      initialBias = 440,
      gainMap     = Encoder.getMap("freqGain"),
      scaling     = app.octaveScaling
    },
    rise = GainBias {
      button      = "rise",
      branch      = self.branches.rise,
      description = "Rise Time",
      gainbias    = self.objects.rise,
      range       = self.objects.rise,
      biasMap     = self.linMap(0, 10, 0.1, 0.01, 0.001, 0.001),
      biasUnits   = app.unitSecs,
      initialBias = 0.001
    },
    fall = GainBias {
      button      = "fall",
      branch      = self.branches.fall,
      description = "Fall Time",
      gainbias    = self.objects.fall,
      range       = self.objects.fall,
      biasMap     = self.linMap(0, 10, 0.1, 0.01, 0.001, 0.001),
      biasUnits   = app.unitSecs,
      initialBias = 0.200
    }
  }, {
    scope     = { "env", "gate", "height", "rise", "fall" },
    expanded  = { "gate", "height", "rise", "fall" },
    collapsed = {}
  }
end

return Lift
