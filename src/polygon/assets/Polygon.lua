local app     = app
local polygon = require "polygon.libpolygon"

local Class   = require "Base.Class"
local Encoder = require "Encoder"

local Unit        = require "Unit"
local Gate        = require "Unit.ViewControl.Gate"
local GainBias    = require "Unit.ViewControl.GainBias"
local Pitch       = require "Unit.ViewControl.Pitch"
local OutputScope = require "Unit.ViewControl.OutputScope"

local OutputMeter    = require "polygon.OutputMeter"
local RoundRobinGate  = require "polygon.RoundRobinGate"
local RoundRobinPitch = require "polygon.RoundRobinPitch"

local Polygon = Class {}
Polygon:include(Unit)

function Polygon:init(args)
  self.ctor = args.ctor
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

function Polygon:addMonitorBranch(name, obj, inlet)
  local monitor = self:addObject(name.."Monitor", app.Monitor())
  connect(monitor, "Out", obj, inlet)
  return self:addMonoBranch(name, monitor, "In", monitor, "Out")
end

function Polygon:addParameterAdapterBranch(name, obj, inlet)
  local pa = self:addObject(name, app.ParameterAdapter())
  pa:hardSet("Gain", 1)
  tie(obj, inlet, pa, "Out")
  return self:addMonoBranch(name, pa, "In", pa, "Out")
end

function Polygon:addVoiceControl(n, op)
  self.voices = self.voices or {}

  self.voices[n] = {
    gateBranch  = self:addMonitorBranch("gate"..n, op, "Gate"..n),
    pitchBranch = self:addParameterAdapterBranch("vpo"..n, op, "V/Oct"..n),
    pitchOffset = op:getParameter("V/Oct Offset"..n)
  }
end

function Polygon:onLoadGraph(channelCount)
  local op = self:addObject("op", self.ctor())

  for i = 1, op:voices() do
    self:addVoiceControl(i, op)
  end

  local pf0      = self:addGainBiasControl("pf0")
  local peak     = self:addGainBiasControl("peak")
  local rise     = self:addParameterAdapterControl("rise")
  local fall     = self:addParameterAdapterControl("fall")
  connect(pf0, "Out", op, "Pitch Fundamental")
  connect(peak, "Out", op, "Peak")
  tie(op, "Rise", rise, "Out")
  tie(op, "Fall", fall, "Out")

  local rrGate   = self:addMonitorBranch("rrGate", op, "RR Gate")
  local rrVpo    = self:addParameterAdapterBranch("rrVpo", op, "RR V/Oct")
  --local rrVpo    = self:addParameterAdapterControl("rrVpo", 1)
  local rrCount  = self:addParameterAdapterControl("rrCount")
  local rrStride = self:addParameterAdapterControl("rrStride")
  local rrTotal  = self:addParameterAdapterControl("rrTotal")
  --tie(op, "RR V/Oct",  rrVpo,    "Out")
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
    wave1 = OutputScope {
      monitor = self,
      width   = 1 * app.SECTION_PLY
    },
    rrGate = RoundRobinGate {
      name    = "Gates",
      polygon = self.objects.op,
      branch  = self.branches.rrGate,
      voices  = self.voices
    },
    rrVpo = RoundRobinPitch {
      name    = "V/Oct",
      polygon = self.objects.op,
      branch  = self.branches.rrVpo,
      tune    = self.objects.rrVpo:getParameter("Bias"),
      voices  = self.voices,
      biasMap = Encoder.getMap("cents")
    },
    count   = GainBias {
      button        = "count",
      description   = "RR Count",
      branch        = self.branches.rrCount,
      gainbias      = self.objects.rrCount,
      range         = self.objects.rrCount,
      biasMap       = self.intMap(1, self.objects.op:voices()),
      biasUnits     = app.unitNone,
      biasPrecision = 0,
      initialBias   = 1
    },
    stride   = GainBias {
      button        = "stride",
      description   = "RR Stride",
      branch        = self.branches.rrStride,
      gainbias      = self.objects.rrStride,
      range         = self.objects.rrStride,
      biasMap       = self.intMap(1, self.objects.op:voices()),
      biasUnits     = app.unitNone,
      biasPrecision = 0,
      initialBias   = 1
    },
    total   = GainBias {
      button        = "total",
      description   = "RR Total",
      branch        = self.branches.rrTotal,
      gainbias      = self.objects.rrTotal,
      range         = self.objects.rrTotal,
      biasMap       = self.intMap(1, self.objects.op:voices()),
      biasUnits     = app.unitNone,
      biasPrecision = 0,
      initialBias   = self.objects.op:voices()
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
    peak = GainBias {
      button      = "peak",
      description = "LPG Peak",
      branch      = self.branches.peak,
      gainbias    = self.objects.peak,
      range       = self.objects.peak,
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
    -- rrVpo = Pitch {
    --   button      = "rrVpo",
    --   branch      = self.branches.rrVpo,
    --   description = "Round Robin V/Oct",
    --   offset      = self.objects.rrVpo,
    --   range       = self.objects.rrVpo
    -- },
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
    expanded  = { "rrGate", "rrVpo", "shape", "fall", "output" },
    rrGate    = { "rrGate", "wave1", "count", "stride", "total" },
    rrVpo     = { "rrVpo", "wave1", "detune", "level", "levelEnv" },
    shape     = { "shape", "wave1", "shapeEnv" },
    fall      = { "fall", "wave1", "rise", "peak" },
    output    = { "output", "wave1", "panOffset", "panWidth" },
    collapsed = { }
  }
end

return Polygon
