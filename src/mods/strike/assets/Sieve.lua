local app = app
local strike = require "strike.libstrike"
local Class = require "Base.Class"
local Encoder = require "Encoder"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Pitch = require "Unit.ViewControl.Pitch"
local UnitShared = require "shared.UnitShared"

local Sieve = Class {}
Sieve:include(Unit)
Sieve:include(UnitShared)

function Sieve:init(args)
  args.title = "Sieve"
  args.mnemonic = "svf"
  Unit.init(self, args)
end

function Sieve:onLoadGraph(channelCount)
  local vpo   = self:addConstantOffsetControl("vpo")
  local f0    = self:addGainBiasControl("f0")
  local q     = self:addGainBiasControl("q")
  local gain  = self:addGainBiasControl("gain")
  local mix   = self:addGainBiasControl("mix")

  local op = self:addObject("op", strike.Sieve())
  connect(vpo,  "Out", op, "V/Oct")
  connect(f0,   "Out", op, "Fundamental")
  connect(q,    "Out", op, "Resonance")
  connect(gain, "Out", op, "Gain")
  connect(mix,  "Out", op, "Mix")

  for i = 1, channelCount do
    connect(self, "In"..i, op, "In"..i)
    connect(op, "Out"..i, self, "Out"..i)
  end
end

function Sieve:onLoadViews()
  return {
    vpo = Pitch {
      button      = "V/oct",
      branch      = self.branches.vpo,
      description = "V/oct",
      offset      = self.objects.vpo,
      range       = self.objects.vpoRange
    },
    f0 = GainBias {
      button      = "f0",
      description = "Fundamental",
      branch      = self.branches.f0,
      gainbias    = self.objects.f0,
      range       = self.objects.f0Range,
      biasMap     = Encoder.getMap("oscFreq"),
      biasUnits   = app.unitHertz,
      initialBias = 440,
      gainMap     = Encoder.getMap("freqGain"),
      scaling     = app.octaveScaling
    },
    q = GainBias {
      button        = "Q",
      description   = "Resonance",
      branch        = self.branches.q,
      gainbias      = self.objects.q,
      range         = self.objects.qRange,
      biasMap       = Encoder.getMap("[0,1]"),
      gainMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 3,
      initialBias   = 0
    },
    gain = GainBias {
      button        = "gain",
      description   = "Input Gain",
      branch        = self.branches.gain,
      gainbias      = self.objects.gain,
      range         = self.objects.gainRange,
      biasMap       = self.linMap(  0, 10, 0.1, 0.01, 0.001, 0.001),
      gainMap       = self.linMap(-10, 10, 0.1, 0.01, 0.001, 0.001),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 1
    },
    mix = GainBias {
      button        = "mix",
      description   = "LP - BP - HP",
      branch        = self.branches.mix,
      gainbias      = self.objects.mix,
      range         = self.objects.mixRange,
      biasMap       = Encoder.getMap("[0,1]"),
      gainMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 3,
      initialBias   = 0
    }
  }, {
    expanded  = { "vpo", "f0",  "q", "gain", "mix" },
    collapsed = { "mix" }
  }
end

return Sieve
