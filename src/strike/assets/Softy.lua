local app = app
local strike = require "strike.libstrike"
local Class = require "Base.Class"
local Unit = require "Unit"
local Encoder = require "Encoder"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"
local Pitch = require "Unit.ViewControl.Pitch"
local GainBias = require "Unit.ViewControl.GainBias"

local Softy = Class {}
Softy:include(Unit)

function Softy:init(args)
  args.title = "Softy"
  args.mnemonic = "sft"
  Unit.init(self, args)
end

function Softy.addComparatorControl(self, name, mode, default)
  local gate = self:addObject(name, app.Comparator())
  gate:setMode(mode)
  self:addMonoBranch(name, gate, "In", gate, "Out")
  if default then
    gate:setOptionValue("State", default)
  end
  return gate
end

function Softy.addGainBiasControl(self, name)
  local gb    = self:addObject(name, app.GainBias());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(gb, "Out", range, "In")
  self:addMonoBranch(name, gb, "In", gb, "Out")
  return gb;
end

function Softy.addConstantOffsetControl(self, name)
  local co    = self:addObject(name, app.ConstantOffset());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(co, "Out", range, "In")
  self:addMonoBranch(name, co, "In", co, "Out")
  return co;
end

function Softy:onLoadGraph(channelCount)
  local tune  = self:addConstantOffsetControl("tune")
  local freq  = self:addGainBiasControl("freq")
  local sync  = self:addComparatorControl("sync", app.COMPARATOR_TRIGGER_ON_RISE)
  local width = self:addGainBiasControl("width")
  local gain  = self:addGainBiasControl("gain")

  local op = self:addObject("op", strike.Softy())
  connect(tune,  "Out", op, "V/Oct")
  connect(freq,  "Out", op, "Frequency")
  connect(sync,  "Out", op, "Sync")
  connect(width, "Out", op, "Width")
  connect(gain,  "Out", op, "Gain")

  for i = 1, channelCount do
    connect(op, "Out", self, "Out"..i)
  end
end

function Softy:onLoadViews()
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
      button      = "sync",
      description = "Sync",
      branch      = self.branches.sync,
      comparator  = self.objects.sync
    },
    width   = GainBias {
      button        = "width",
      description   = "Width",
      branch        = self.branches.width,
      gainbias      = self.objects.width,
      range         = self.objects.widthRange,
      biasMap       = Encoder.getMap("[0,1]"),
      gainMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0.5
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
    }
  }, {
    expanded  = { "tune", "freq", "sync", "width", "gain" },
    collapsed = { }
  }
end

return Softy
