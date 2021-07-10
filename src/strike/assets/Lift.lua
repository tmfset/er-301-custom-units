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

local Lift = Class {}
Lift:include(Unit)

function Lift:init(args)
  args.title = "Lift"
  args.mnemonic = "lpg"
  Unit.init(self, args)
end

function Lift.addComparatorControl(self, name, mode, default)
  local gate = self:addObject(name, app.Comparator())
  gate:setMode(mode)
  self:addMonoBranch(name, gate, "In", gate, "Out")
  if default then
    gate:setOptionValue("State", default)
  end
  return gate
end

function Lift.addGainBiasControlNoBranch(self, name)
  local gb    = self:addObject(name, app.GainBias());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(gb, "Out", range, "In")
  return gb;
end

function Lift.addGainBiasControl(self, name)
  local gb = self:addGainBiasControlNoBranch(name);
  self:addMonoBranch(name, gb, "In", gb, "Out")
  return gb;
end

function Lift.addConstantOffsetControl(self, name)
  local co    = self:addObject(name, app.ConstantOffset());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(co, "Out", range, "In")
  self:addMonoBranch(name, co, "In", co, "Out")
  return co;
end

function Lift.addParameterAdapterControl(self, name)
  local pa = self:addObject(name, app.ParameterAdapter())
  self:addMonoBranch(name, pa, "In", pa, "Out")
  return pa
end

function Lift.linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

function Lift:onLoadGraph(channelCount)
  local stereo = channelCount > 1

  local gate   = self:addComparatorControl("gate", app.COMPARATOR_GATE)
  local rise   = self:addParameterAdapterControl("rise")
  local fall   = self:addParameterAdapterControl("fall")
  local height = self:addParameterAdapterControl("height")

  local op = self:addObject("op", strike.Lift(stereo))
  tie(op, "Rise", rise, "Out")
  tie(op, "Fall", fall, "Out")
  tie(op, "Height", height, "Out")
  connect(gate, "Out", op, "Gate")

  for i = 1, channelCount do
    connect(self, "In"..i, op, "In"..i)
    connect(op, "Out"..i, self, "Out"..i)
  end

  self:addMonoBranch("height", height, "In", self.objects.op, "Env")
end

function Lift:onLoadViews()
  return {
    gate = Gate {
      button      = "gate",
      description = "Gate",
      branch      = self.branches.gate,
      comparator  = self.objects.gate
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
    }
  }, {
    expanded  = { "gate", "height", "rise", "fall" },
    collapsed = {}
  }
end

return Lift
