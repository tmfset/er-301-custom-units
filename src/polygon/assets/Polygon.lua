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

function Polygon:onLoadGraph(channelCount)
  local gate   = self:addComparatorControl("gate", app.COMPARATOR_GATE)
  local vpo    = self:addConstantOffsetControl("vpo")
  local detune = self:addConstantOffsetControl("detune")
  local f0     = self:addGainBiasControl("f0")
  local rise   = self:addGainBiasControl("rise")
  local fall   = self:addGainBiasControl("fall")

  local op = self:addObject("op", polygon.Polygon())
  connect(gate,   "Out", op, "Gate")
  connect(vpo,    "Out", op, "V/Oct")
  connect(detune, "Out", op, "Detune")
  connect(f0,     "Out", op, "Fundamental")
  connect(rise,   "Out", op, "Rise")
  connect(fall,   "Out", op, "Fall")

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
    detune = Pitch {
      button      = "detune",
      branch      = self.branches.detune,
      description = "Detune",
      offset      = self.objects.detune,
      range       = self.objects.detuneRange
    },
    f0 = GainBias {
      button      = "f0",
      description = "Frequency",
      branch      = self.branches.f0,
      gainbias    = self.objects.f0,
      range       = self.objects.f0Range,
      biasMap     = Encoder.getMap("oscFreq"),
      biasUnits   = app.unitHertz,
      initialBias = 27.5,
      gainMap     = Encoder.getMap("freqGain"),
      scaling     = app.octaveScaling
    },
    rise = GainBias {
      button      = "rise",
      branch      = self.branches.rise,
      description = "Rise Time",
      gainbias    = self.objects.rise,
      range       = self.objects.riseRange,
      biasMap     = self.linMap(0, 10, 0.1, 0.01, 0.001, 0.001),
      biasUnits   = app.unitSecs,
      initialBias = 0.005
    },
    fall = GainBias {
      button      = "fall",
      branch      = self.branches.fall,
      description = "Fall Time",
      gainbias    = self.objects.fall,
      range       = self.objects.fallRange,
      biasMap     = self.linMap(0, 10, 0.1, 0.01, 0.001, 0.001),
      biasUnits   = app.unitSecs,
      initialBias = 0.500
    }
  }, {
    expanded  = { "gate", "vpo", "detune", "f0", "rise", "fall" },
    collapsed = { }
  }
end

return Polygon
