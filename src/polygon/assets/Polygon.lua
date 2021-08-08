local app = app
local polygon = require "polygon.libpolygon"
local Class = require "Base.Class"
local Encoder = require "Encoder"
local Unit = require "Unit"
local Gate = require "Unit.ViewControl.Gate"
local GainBias = require "Unit.ViewControl.GainBias"
local Pitch = require "Unit.ViewControl.Pitch"
local OutputMeter = require "polygon.OutputMeter"

local Polygon = Class {}
Polygon:include(Unit)

function Polygon:init(args)
  args.title = "Polygon"
  args.mnemonic = "ply"
  Unit.init(self, args)
end

function Polygon.addComparatorControl(self, name, mode, default)
  local gate = self:addObject(name, app.Comparator())
  gate:setMode(mode)
  self:addMonoBranch(name, gate, "In", gate, "Out")
  if default then
    gate:setOptionValue("State", default)
  end
  return gate
end

function Polygon.addGainBiasControl(self, name)
  local gb    = self:addObject(name, app.GainBias());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(gb, "Out", range, "In")
  self:addMonoBranch(name, gb, "In", gb, "Out")
  return gb;
end

function Polygon.addConstantOffsetControl(self, name)
  local co    = self:addObject(name, app.ConstantOffset());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(co, "Out", range, "In")
  self:addMonoBranch(name, co, "In", co, "Out")
  return co;
end

function Polygon.addParameterAdapterControl(self, name)
  local pa = self:addObject(name, app.ParameterAdapter())
  self:addMonoBranch(name, pa, "In", pa, "Out")
  return pa
end

function Polygon:onLoadGraph(channelCount)
  local gateRR      = self:addComparatorControl("gateRR", app.COMPARATOR_GATE)
  local vpoRR       = self:addParameterAdapterControl("vpoRR")
  local detuneRR    = self:addParameterAdapterControl("detuneRR")
  local cutoffRR    = self:addParameterAdapterControl("cutoffRR")
  local shapeRR     = self:addParameterAdapterControl("shapeRR")
  local levelRR     = self:addParameterAdapterControl("levelRR")
  local panRR       = self:addParameterAdapterControl("panRR")
  vpoRR:hardSet("Gain", 1)
  detuneRR:hardSet("Gain", 1)

  local pitchF0     = self:addGainBiasControl("pitchF0")
  local filterF0    = self:addGainBiasControl("filterF0")

  local rise      = self:addParameterAdapterControl("rise")
  local fall      = self:addParameterAdapterControl("fall")
  local shapeEnv  = self:addParameterAdapterControl("shapeEnv")
  local levelEnv  = self:addParameterAdapterControl("levelEnv")
  local panEnv    = self:addParameterAdapterControl("panEnv")

  local op = self:addObject("op", polygon.Polygon())
  connect(gateRR,   "Out", op, "Gate RR")
  connect(pitchF0,  "Out", op, "Pitch Fundamental")
  connect(filterF0, "Out", op, "Filter Fundamental")

  tie(op, "V/Oct RR",  vpoRR,    "Out")
  tie(op, "Detune RR", detuneRR, "Out")
  tie(op, "Cutoff RR", cutoffRR, "Out")
  tie(op, "Shape RR",  shapeRR,  "Out")
  tie(op, "Level RR",  levelRR,  "Out")
  tie(op, "Pan RR",    panRR,    "Out")

  tie(op, "Rise",       rise,      "Out")
  tie(op, "Fall",       fall,      "Out")
  tie(op, "Shape Env",  shapeEnv,  "Out")
  tie(op, "Level Env",  levelEnv,  "Out")
  tie(op, "Pan Env",    panEnv,    "Out")

  for i = 1, channelCount do
    connect(op, "Out"..i, self, "Out"..i)
  end
end

function Polygon.linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

function Polygon.defaultDecibelMap()
  local map = app.LinearDialMap(-60, 12)
  map:setZero(0)
  map:setSteps(6, 1, 0.1, 0.01);
  return map
end

function Polygon:onLoadViews()
  return {
    gate = Gate {
      button      = "gate",
      description = "Gate",
      branch      = self.branches.gateRR,
      comparator  = self.objects.gateRR
    },
    vpo = Pitch {
      button      = "V/oct",
      branch      = self.branches.vpoRR,
      description = "V/oct",
      offset      = self.objects.vpoRR,
      range       = self.objects.vpoRR
    },
    detune = Pitch {
      button      = "V/oct",
      branch      = self.branches.detuneRR,
      description = "V/oct",
      offset      = self.objects.detuneRR,
      range       = self.objects.detuneRR
    },
    pan   = GainBias {
      button        = "pan",
      description   = "Pan",
      branch        = self.branches.panRR,
      gainbias      = self.objects.panRR,
      range         = self.objects.panRR,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0
    },
    pf0 = GainBias {
      button      = "pf0",
      description = "Pitch Fundamental",
      branch      = self.branches.pitchF0,
      gainbias    = self.objects.pitchF0,
      range       = self.objects.pitchF0,
      biasMap     = Encoder.getMap("oscFreq"),
      biasUnits   = app.unitHertz,
      initialBias = 110,
      gainMap     = Encoder.getMap("freqGain"),
      scaling     = app.octaveScaling
    },
    ff0 = GainBias {
      button      = "ff0",
      description = "Filter Fundamental",
      branch      = self.branches.filterF0,
      gainbias    = self.objects.filterF0,
      range       = self.objects.filterF0,
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
    },
    shape   = GainBias {
      button        = "shape",
      description   = "Shape",
      branch        = self.branches.shapeRR,
      gainbias      = self.objects.shapeRR,
      range         = self.objects.shapeRR,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0
    },
    level   = GainBias {
      button        = "subLvl",
      description   = "Sub Level",
      branch        = self.branches.levelRR,
      gainbias      = self.objects.levelRR,
      range         = self.objects.levelRR,
      biasMap       = Encoder.getMap("[0,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0.5
    },
    output = OutputMeter {
      button       = "output",
      description  = "Output Gain",
      polygon      = self.objects.op,
      channelCount = self.channelCount,
      map          = self.defaultDecibelMap(),
      units        = app.unitDecibels,
      scaling      = app.linearScaling
    }
  }, {
    expanded  = { "output", "pan", "gate", "vpo", "detune", "pf0", "ff0", "rise", "fall", "shape", "level" },
    collapsed = { }
  }
end

return Polygon
