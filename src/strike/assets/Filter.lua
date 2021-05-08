local app = app
local strike = require "strike.libstrike"
local Class = require "Base.Class"
local Encoder = require "Encoder"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Pitch = require "Unit.ViewControl.Pitch"

local Filter = Class {}
Filter:include(Unit)

function Filter:init(args)
  args.title = "Filter"
  args.mnemonic = "F"
  Unit.init(self, args)
end

function Filter.addGainBiasControl(self, name)
  local gb    = self:addObject(name, app.GainBias());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(gb, "Out", range, "In")
  self:addMonoBranch(name, gb, "In", gb, "Out")
  return gb;
end

function Filter.addConstantOffsetControl(self, name)
  local co    = self:addObject(name, app.ConstantOffset());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(co, "Out", range, "In")
  self:addMonoBranch(name, co, "In", co, "Out")
  return co;
end

function Filter:onLoadGraph(channelCount)
  local vpo   = self:addConstantOffsetControl("vpo")
  local f0    = self:addGainBiasControl("f0")
  local q     = self:addGainBiasControl("q")
  local gain  = self:addGainBiasControl("gain")

  local op = self:addObject("op", strike.Filter())
  connect(vpo,  "Out", op, "V/Oct")
  connect(f0,   "Out", op, "Fundamental")
  connect(q,    "Out", op, "Resonance")
  connect(gain, "Out", op, "Gain")

  connect(self, "In1", op, "Left In")
  connect(op, "Left Out", self, "Out1")

  if channelCount > 1 then
    connect(self, "In2", op, "Right In")
    connect(op, "Right Out", self, "Out2")
  end
end

function Filter:onLoadViews()
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
      biasMap       = Encoder.getMap("[0,10]"),
      gainMap       = Encoder.getMap("[-10,10]"),
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
      biasMap       = Encoder.getMap("[0,10]"),
      biasUnits     = app.unitNone,
      biasPrecision = 3,
      initialBias   = 1
    },
  }, {
    expanded  = { "vpo", "f0",  "q", "gain" },
    collapsed = {}
  }
end

return Filter
