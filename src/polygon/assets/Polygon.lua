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
  self.isStereo = channelCount == 2

  local op = self:addObject("op", self.ctor())

  for i = 1, op:voices() do
    self:addVoiceControl(i, op)
  end

  local pf0      = self:addGainBiasControl("pf0")
  local ff0      = self:addGainBiasControl("ff0")
  local rise     = self:addParameterAdapterControl("rise")
  local fall     = self:addParameterAdapterControl("fall")
  connect(pf0, "Out", op, "Pitch Fundamental")
  connect(ff0, "Out", op, "Filter Fundamental")
  tie(op, "Rise", rise, "Out")
  tie(op, "Fall", fall, "Out")

  local gate   = self:addMonitorBranch("gate", op, "RR Gate")
  local vpo    = self:addParameterAdapterBranch("vpo", op, "RR V/Oct")
  local count  = self:addParameterAdapterControl("count")
  local stride = self:addParameterAdapterControl("stride")
  local total  = self:addParameterAdapterControl("total")
  tie(op, "RR Count",  count,  "Out")
  tie(op, "RR Stride", stride, "Out")
  tie(op, "RR Total",  total,  "Out")

  local fvpo     = self:addParameterAdapterControl("fvpo", 1)
  local resonance = self:addParameterAdapterControl("resonance")
  local detune   = self:addParameterAdapterControl("detune", 1)
  local level    = self:addParameterAdapterControl("level")
  local shape    = self:addParameterAdapterControl("shape")
  tie(op, "Filter V/Oct", fvpo, "Out")
  tie(op, "Resonance", resonance, "Out")
  tie(op, "Detune", detune, "Out")
  tie(op, "Level",  level,  "Out")
  tie(op, "Shape",  shape,  "Out")

  local lenv = self:addParameterAdapterControl("lenv")
  local senv = self:addParameterAdapterControl("senv")
  local fenv = self:addParameterAdapterControl("fenv")
  tie(op,  "Level Env", lenv, "Out")
  tie(op,  "Shape Env", senv, "Out")
  tie(op, "Filter Env", fenv, "Out")

  if self.isStereo then
    local pan   = self:addParameterAdapterControl("pan")
    local width = self:addParameterAdapterControl("width")
    tie(op, "Pan Offset", pan,   "Out")
    tie(op, "Pan Width",  width, "Out")
  else
    op:getParameter("Pan Offset"):hardSet(-1)
    op:getParameter("Pan Width"):hardSet(0)
  end

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
  local views = {
    scope     = { "gate", "vpo", "pf0", "ff0", "shape", "detune", "rise", "fall" },
    expanded  = { "gate", "vpo", "shape", "detune", "fall", "ff0", "output" },
    collapsed = { "gate", "vpo" },

    gate      = { "gate",   "wave1", "count", "stride", "total" },
    vpo       = { "vpo",    "wave1", "pf0" },
    shape     = { "shape",  "wave1", "senv" },
    detune    = { "detune", "wave1", "level", "lenv" },
    fall      = { "fall",   "wave1", "rise" },
    ff0       = { "ff0",    "wave1", "fvpo", "fenv", "resonance" }
  }

  local controls = {
    wave1 = OutputScope {
      monitor = self,
      width   = 1 * app.SECTION_PLY
    },
    count   = GainBias {
      button        = "count",
      description   = "RR Count",
      branch        = self.branches.count,
      gainbias      = self.objects.count,
      range         = self.objects.count,
      biasMap       = self.intMap(1, self.objects.op:voices()),
      biasUnits     = app.unitNone,
      biasPrecision = 0,
      initialBias   = 1
    },
    stride   = GainBias {
      button        = "stride",
      description   = "Stride",
      branch        = self.branches.stride,
      gainbias      = self.objects.stride,
      range         = self.objects.stride,
      biasMap       = self.intMap(1, self.objects.op:voices()),
      biasUnits     = app.unitNone,
      biasPrecision = 0,
      initialBias   = 1
    },
    total   = GainBias {
      button        = "total",
      description   = "Total",
      branch        = self.branches.total,
      gainbias      = self.objects.total,
      range         = self.objects.total,
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
      initialBias = 0
    },
    fall = GainBias {
      button      = "fall",
      branch      = self.branches.fall,
      description = "Fall Time",
      gainbias    = self.objects.fall,
      range       = self.objects.fall,
      biasMap     = self.linMap(0, 10, 0.1, 0.01, 0.001, 0.001),
      biasUnits   = app.unitSecs,
      initialBias = 0.500
    },
    fvpo   = Pitch {
      button      = "fvpo",
      branch      = self.branches.fvpo,
      description = "Filter V/Oct",
      offset      = self.objects.fvpo,
      range       = self.objects.fvpo
    },
    resonance   = GainBias {
      button        = "res",
      description   = "Resonance",
      branch        = self.branches.resonance,
      gainbias      = self.objects.resonance,
      range         = self.objects.resonance,
      biasMap       = Encoder.getMap("[0,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0
    },
    detune = Pitch {
      button      = "detune",
      branch      = self.branches.detune,
      description = "Detune V/Oct",
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
      initialBias   = 1
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
    lenv   = GainBias {
      button        = "lenv",
      description   = "Level Env",
      branch        = self.branches.lenv,
      gainbias      = self.objects.lenv,
      range         = self.objects.lenv,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0
    },
    senv   = GainBias {
      button        = "senv",
      description   = "Shape Env",
      branch        = self.branches.senv,
      gainbias      = self.objects.senv,
      range         = self.objects.senv,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = -0.2
    },
    fenv   = GainBias {
      button        = "fenv",
      description   = "Filter Env",
      branch        = self.branches.fenv,
      gainbias      = self.objects.fenv,
      range         = self.objects.fenv,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0.2
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
  }

  controls.gate = RoundRobinGate {
    name    = "gate",
    polygon = self.objects.op,
    branch  = self.branches.gate,
    voices  = self.voices
  }

  controls.vpo = RoundRobinPitch {
    name    = "vpo",
    polygon = self.objects.op,
    branch  = self.branches.vpo,
    tune    = self.objects.vpo:getParameter("Bias"),
    voices  = self.voices,
    biasMap = Encoder.getMap("cents")
  }

  controls.gate:attachFollower(controls.vpo)
  controls.vpo:attachFollower(controls.gate)

  if self.isStereo then
    controls.pan = GainBias {
      button        = "pan",
      description   = "Pan Offset",
      branch        = self.branches.pan,
      gainbias      = self.objects.pan,
      range         = self.objects.pan,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0
    }

    controls.width = GainBias {
      button        = "width",
      description   = "Pan Width",
      branch        = self.branches.width,
      gainbias      = self.objects.width,
      range         = self.objects.width,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0.2
    }

    views.output = { "output", "wave1", "pan", "width" }
    views.scope[#views.scope + 1] = "pan"
    views.scope[#views.scope + 1] = "width"
  else
    views.output = { "output", "wave1" }
  end

  return controls, views
end

return Polygon
