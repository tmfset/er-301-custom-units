local app = app
local strike = require "strike.libstrike"
local Class = require "Base.Class"
local Encoder = require "Encoder"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Pitch = require "Unit.ViewControl.Pitch"
local MOptionControl = require "Unit.MenuControl.OptionControl"
local DualOptionControl = require "strike.DualOptionControl"
local Common = require "strike.Common"

local Bique = Class {}
Bique:include(Unit)
Bique:include(Common)

function Bique:init(args)
  args.title = "Bique"
  args.mnemonic = "bcf"
  Unit.init(self, args)
end

function Bique:onLoadGraph(channelCount)
  local stereo = channelCount > 1

  local vpo   = self:addConstantOffsetControl("vpo")
  local f0    = self:addGainBiasControl("f0")
  local q     = self:addGainBiasControl("q")
  local gain  = self:addGainBiasControl("gain")

  local op = self:addObject("op", strike.Bique(stereo))
  connect(vpo,  "Out", op, "V/Oct")
  connect(f0,   "Out", op, "Fundamental")
  connect(q,    "Out", op, "Resonance")
  connect(gain, "Out", op, "Gain")

  for i = 1, stereo and 2 or 1 do
    connect(self, "In"..i, op, "In"..i)
    connect(op, "Out"..i, self, "Out"..i)
  end
end

function Bique:onLoadViews()
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
    type = DualOptionControl {
      button       = "type",
      descriptionA = "Type",
      optionA      = self.objects.op:getOption("Type"),
      choicesA     = { "LP", "BP", "HP" },
      descriptionB = "Mode",
      optionB      = self.objects.op:getOption("Mode"),
      choicesB     = { "12dB", "24dB", "36dB" },
      muteOnChange = true
    }
  }, {
    expanded  = { "type", "vpo", "f0",  "q", "gain" },
    collapsed = { "type" }
  }
end

return Bique
