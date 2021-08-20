local app = app
local polygon = require "polygon.libpolygon"
local Class = require "Base.Class"
local Encoder = require "Encoder"
local Unit = require "Unit"
local Gate = require "Unit.ViewControl.Gate"
local GainBias = require "Unit.ViewControl.GainBias"
local Pitch = require "Unit.ViewControl.Pitch"
local RoundRobin = require "polygon.RoundRobin"
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

function Polygon.addParameterAdapterControl(self, name, defaultGain)
  local pa = self:addObject(name, app.ParameterAdapter())
  self:addMonoBranch(name, pa, "In", pa, "Out")
  if defaultGain then
    pa:hardSet("Gain", defaultGain)
  end
  return pa
end

function Polygon:onLoadGraph(channelCount)
  local op = self:addObject("op", polygon.Polygon(channelCount > 1))

  local pf0      = self:addGainBiasControl("pf0")
  local ff0      = self:addGainBiasControl("ff0")
  local rise     = self:addParameterAdapterControl("rise")
  local fall     = self:addParameterAdapterControl("fall")
  connect(pf0, "Out", op, "Pitch Fundamental")
  connect(ff0, "Out", op, "Filter Fundamental")
  tie(op, "Rise", rise, "Out")
  tie(op, "Fall", fall, "Out")

  local rrGate   = self:addComparatorControl("rrGate", app.COMPARATOR_GATE)
  local rrVpo    = self:addParameterAdapterControl("rrVpo", 1)
  local rrCount  = self:addParameterAdapterControl("rrCount")
  local rrStride = self:addParameterAdapterControl("rrStride")
  local rrTotal  = self:addParameterAdapterControl("rrTotal")
  connect(rrGate, "Out", op, "RR Gate")
  tie(op, "RR V/Oct",  rrVpo,    "Out")
  tie(op, "RR Count",  rrCount,  "Out")
  tie(op, "RR Stride", rrStride, "Out")
  tie(op, "RR Total",  rrTotal,  "Out")

  local detune   = self:addParameterAdapterControl("detune", 1)
  local level    = self:addParameterAdapterControl("level")
  local levelEnv = self:addParameterAdapterControl("levelEnv")
  tie(op, "Detune",    detune,   "Out")
  tie(op, "Level",     level,    "Out")
  tie(op, "Level Env", levelEnv, "Out")

  local shape    = self:addParameterAdapterControl("shape")
  local shapeEnv = self:addParameterAdapterControl("shapeEnv")
  tie(op, "Shape",     shape,    "Out")
  tie(op, "Shape Env", shapeEnv, "Out")

  local panOffset = self:addParameterAdapterControl("panOffset")
  local panWidth  = self:addParameterAdapterControl("panWidth")
  tie(op, "Pan Offset", panOffset, "Out")
  tie(op, "Pan Width",  panWidth,  "Out")

  for i = 1, channelCount do
    connect(op, "Out"..i, self, "Out"..i)
  end
end

function Polygon.linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

function Polygon.intMap(min, max)
  local map = app.LinearDialMap(min,max)
    map:setSteps(2, 1, 0.25, 0.25)
    map:setRounding(1)
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
    roundRobin = RoundRobin {
      polygon = self.objects.op
    },
    pf0 = GainBias {
      button      = "pf0",
      description = "Pitch Fundamental",
      branch      = self.branches.pf0,
      gainbias    = self.objects.pf0,
      range       = self.objects.pf0,
      biasMap     = Encoder.getMap("oscFreq"),
      biasUnits   = app.unitHertz,
      initialBias = 110,
      gainMap     = Encoder.getMap("freqGain"),
      scaling     = app.octaveScaling
    },
    ff0 = GainBias {
      button      = "ff0",
      description = "Filter Fundamental",
      branch      = self.branches.ff0,
      gainbias    = self.objects.ff0,
      range       = self.objects.ff0,
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
    rrGate = Gate {
      button      = "rrGate",
      description = "Round Robin Gate",
      branch      = self.branches.rrGate,
      comparator  = self.objects.rrGate
    },
    rrVpo = Pitch {
      button      = "rrVpo",
      branch      = self.branches.rrVpo,
      description = "Round Robin V/Oct",
      offset      = self.objects.rrVpo,
      range       = self.objects.rrVpo
    },
    rrCount   = GainBias {
      button        = "rrCount",
      description   = "Round Robin Count",
      branch        = self.branches.rrCount,
      gainbias      = self.objects.rrCount,
      range         = self.objects.rrCount,
      biasMap       = self.intMap(1, 8),
      biasUnits     = app.unitNone,
      biasPrecision = 0,
      initialBias   = 1
    },
    rrStride   = GainBias {
      button        = "rrStride",
      description   = "Round Robin Stride",
      branch        = self.branches.rrStride,
      gainbias      = self.objects.rrStride,
      range         = self.objects.rrStride,
      biasMap       = self.intMap(1, 8),
      biasUnits     = app.unitNone,
      biasPrecision = 0,
      initialBias   = 1
    },
    rrTotal   = GainBias {
      button        = "rrTotal",
      description   = "Round Robin Total",
      branch        = self.branches.rrTotal,
      gainbias      = self.objects.rrTotal,
      range         = self.objects.rrTotal,
      biasMap       = self.intMap(1, 8),
      biasUnits     = app.unitNone,
      biasPrecision = 0,
      initialBias   = 8
    },
    detune = Pitch {
      button      = "detune",
      branch      = self.branches.detune,
      description = "Detune V/oct",
      offset      = self.objects.detune,
      range       = self.objects.detune
    },
    level   = GainBias {
      button        = "subLvl",
      description   = "Sub Level",
      branch        = self.branches.level,
      gainbias      = self.objects.level,
      range         = self.objects.level,
      biasMap       = Encoder.getMap("[0,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0.5
    },
    levelEnv   = GainBias {
      button        = "lvlEnv",
      description   = "Level Env",
      branch        = self.branches.levelEnv,
      gainbias      = self.objects.levelEnv,
      range         = self.objects.levelEnv,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0
    },
    shape   = GainBias {
      button        = "shape",
      description   = "Shape",
      branch        = self.branches.shape,
      gainbias      = self.objects.shape,
      range         = self.objects.shape,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0
    },
    shapeEnv   = GainBias {
      button        = "shpEnv",
      description   = "Shape Env",
      branch        = self.branches.shapeEnv,
      gainbias      = self.objects.shapeEnv,
      range         = self.objects.shapeEnv,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0
    },
    panOffset   = GainBias {
      button        = "panOff",
      description   = "Pan Offset",
      branch        = self.branches.panOffset,
      gainbias      = self.objects.panOffset,
      range         = self.objects.panOffset,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0
    },
    panWidth   = GainBias {
      button        = "panWid",
      description   = "Pan Width",
      branch        = self.branches.panWidth,
      gainbias      = self.objects.panWidth,
      range         = self.objects.panWidth,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0
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
    expanded = { "roundRobin", "rrGate", "rrCount", "rrStride", "rrTotal" },
    --expanded  = { "output", "rrGate", "rrVpo", "rrCount", "rrStride", "rrTotal", "pf0", "ff0", "rise", "fall", "detune", "level", "levelEnv", "shape", "shapeEnv", "panOffset", "panWidth" },
    collapsed = { }
  }
end

return Polygon
