local app = app
local polygon = require "polygon.libpolygon"
local Class = require "Base.Class"
local Encoder = require "Encoder"
local Unit = require "Unit"
local Gate = require "Unit.ViewControl.Gate"
local GainBias = require "Unit.ViewControl.GainBias"
local Pitch = require "Unit.ViewControl.Pitch"

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
  local gate      = self:addComparatorControl("gate", app.COMPARATOR_GATE)
  local vpo       = self:addConstantOffsetControl("vpo")
  local f0        = self:addGainBiasControl("f0")
  local gain      = self:addGainBiasControl("gain")
  local shape     = self:addGainBiasControl("shape")
  local subLevel  = self:addGainBiasControl("subLevel")
  local subDivide = self:addGainBiasControl("subDivide")
  local height    = self:addParameterAdapterControl("height")
  local rise      = self:addParameterAdapterControl("rise")
  local fall      = self:addParameterAdapterControl("fall")

  local op = self:addObject("op", polygon.Polygon(4))
  connect(gate,      "Out", op, "Gate")
  connect(vpo,       "Out", op, "V/Oct")
  connect(f0,        "Out", op, "Fundamental")
  connect(gain,      "Out", op, "Gain")
  connect(shape,     "Out", op, "Shape")
  connect(subLevel,  "Out", op, "Sub Level")
  connect(subDivide, "Out", op, "Sub Divide")

  tie(op, "Rise",   rise,   "Out")
  tie(op, "Fall",   fall,   "Out")
  tie(op, "Height", height, "Out")

  for i = 1, channelCount do
    connect(op, "Out", self, "Out"..i)
  end
end

function Polygon.linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

function Polygon:onLoadViews()
  return {
    gate = Gate {
      button      = "gate",
      description = "Gate",
      branch      = self.branches.gate,
      comparator  = self.objects.gate
    },
    vpo = Pitch {
      button      = "V/oct",
      branch      = self.branches.vpo,
      description = "V/oct",
      offset      = self.objects.vpo,
      range       = self.objects.vpoRange
    },
    f0 = GainBias {
      button      = "f0",
      description = "Frequency",
      branch      = self.branches.f0,
      gainbias    = self.objects.f0,
      range       = self.objects.f0Range,
      biasMap     = Encoder.getMap("oscFreq"),
      biasUnits   = app.unitHertz,
      initialBias = 110,
      gainMap     = Encoder.getMap("freqGain"),
      scaling     = app.octaveScaling
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
    },
    shape   = GainBias {
      button        = "shape",
      description   = "Shape",
      branch        = self.branches.shape,
      gainbias      = self.objects.shape,
      range         = self.objects.shapeRange,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0.5
    },
    subLevel   = GainBias {
      button        = "subLvl",
      description   = "Sub Level",
      branch        = self.branches.subLevel,
      gainbias      = self.objects.subLevel,
      range         = self.objects.subLevelRange,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0.5
    },
    subDivide   = GainBias {
      button        = "subDiv",
      description   = "Sub Level",
      branch        = self.branches.subDivide,
      gainbias      = self.objects.subDivide,
      range         = self.objects.subDivideRange,
      biasMap       = Encoder.getMap("[0,10]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 2
    },
    gain   = GainBias {
      button        = "gain",
      description   = "Gain",
      branch        = self.branches.gain,
      gainbias      = self.objects.gain,
      range         = self.objects.gainRange,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0.5
    }
  }, {
    expanded  = { "gate", "vpo", "f0", "height", "rise", "fall", "shape", "subLevel", "subDivide", "gain" },
    collapsed = { }
  }
end

return Polygon
