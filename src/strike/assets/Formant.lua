local app = app
local strike = require "strike.libstrike"
local Class = require "Base.Class"
local Encoder = require "Encoder"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"
local OutputScope = require "Unit.ViewControl.OutputScope"
local OptionControl = require "Unit.MenuControl.OptionControl"
local Pitch = require "Unit.ViewControl.Pitch"

local Formant = Class {}
Formant:include(Unit)

function Formant:init(args)
  args.title = "Formant"
  args.mnemonic = "fmt"
  Unit.init(self, args)
end

function Formant.addComparatorControl(self, name, mode, default)
  local gate = self:addObject(name, app.Comparator())
  gate:setMode(mode)
  self:addMonoBranch(name, gate, "In", gate, "Out")
  if default then
    gate:setOptionValue("State", default)
  end
  return gate
end

function Formant.addGainBiasControl(self, name)
  local gb    = self:addObject(name, app.GainBias());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(gb, "Out", range, "In")
  self:addMonoBranch(name, gb, "In", gb, "Out")
  return gb;
end

function Formant.addConstantOffsetControl(self, name)
  local co    = self:addObject(name, app.ConstantOffset());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(co, "Out", range, "In")
  self:addMonoBranch(name, co, "In", co, "Out")
  return co;
end

function Formant.linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

function Formant:onLoadGraph(channelCount)
  local stereo = channelCount > 1

  local tune     = self:addConstantOffsetControl("tune")
  local freq     = self:addGainBiasControl("freq")
  local sync     = self:addComparatorControl("sync", app.COMPARATOR_TRIGGER_ON_RISE)
  local formant  = self:addGainBiasControl("formant")
  local gain     = self:addGainBiasControl("gain")
  local barrel   = self:addGainBiasControl("barrel")

  local op = self:addObject("op", strike.Formant())
  connect(tune,     "Out", op, "V/Oct")
  connect(freq,     "Out", op, "Frequency")
  connect(sync,     "Out", op, "Sync")
  connect(formant,  "Out", op, "Formant")
  connect(gain,     "Out", op, "Gain")
  connect(barrel,   "Out", op, "Barrel")

  for i = 1, channelCount do
    connect(op, "Out", self, "Out"..i)
  end
end

function Formant:onLoadViews()
  return {
    tune = Pitch {
      button      = "V/oct",
      branch      = self.branches.tune,
      description = "V/oct",
      offset      = self.objects.tune,
      range       = self.objects.tuneRange
    },
    freq = GainBias {
      button      = "freq",
      description = "Frequency",
      branch      = self.branches.freq,
      gainbias    = self.objects.freq,
      range       = self.objects.freqRange,
      biasMap     = Encoder.getMap("oscFreq"),
      biasUnits   = app.unitHertz,
      initialBias = 27.5,
      gainMap     = Encoder.getMap("freqGain"),
      scaling     = app.octaveScaling
    },
    sync    = Gate {
      button = "sync",
      description = "Sync",
      branch = self.branches.sync,
      comparator = self.objects.sync
    },
    formant   = GainBias {
      button        = "formant",
      description   = "Formant",
      branch        = self.branches.formant,
      gainbias      = self.objects.formant,
      range         = self.objects.formantRange,
      biasMap       = Encoder.getMap("[-1,1]"),
      gainMap       = Encoder.getMap("[-1,1]"),
      biasPrecision = 3,
      initialBias   = 0
    },
    gain   = GainBias {
      button        = "gain",
      description   = "Gain",
      branch        = self.branches.gain,
      gainbias      = self.objects.gain,
      range         = self.objects.gain,
      biasMap       = Encoder.getMap("[-1,1]"),
      gainMap       = Encoder.getMap("[-1,1]"),
      biasPrecision = 2,
      initialBias   = 0.5
    },
    barrel = GainBias {
      button        = "barrel",
      description   = "Barrel",
      branch        = self.branches.barrel,
      gainbias      = self.objects.barrel,
      range         = self.objects.barrelRange,
      biasMap       = Encoder.getMap("[0,1]"),
      gainMap       = Encoder.getMap("[-1,1]"),
      biasPrecision = 3,
      initialBias   = 0.5
    }
  }, {
    expanded  = { "tune", "freq", "sync", "formant", "barrel", "gain" },
    collapsed = {}
  }
end

return Formant
