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

local CPR = Class {}
CPR:include(Unit)

function CPR:init(args)
  args.title = "CPR"
  args.mnemonic = "cpr"
  Unit.init(self, args)
end

function CPR.addComparatorControl(self, name, mode, default)
  local gate = self:addObject(name, app.Comparator())
  gate:setMode(mode)
  self:addMonoBranch(name, gate, "In", gate, "Out")
  if default then
    gate:setOptionValue("State", default)
  end
  return gate
end

function CPR.addGainBiasControlNoBranch(self, name)
  local gb    = self:addObject(name, app.GainBias());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(gb, "Out", range, "In")
  return gb;
end

function CPR.addGainBiasControl(self, name)
  local gb = self:addGainBiasControlNoBranch(name);
  self:addMonoBranch(name, gb, "In", gb, "Out")
  return gb;
end

function CPR.addConstantOffsetControl(self, name)
  local co    = self:addObject(name, app.ConstantOffset());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(co, "Out", range, "In")
  self:addMonoBranch(name, co, "In", co, "Out")
  return co;
end

function CPR.linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

function CPR:onLoadGraph(channelCount)
  local stereo = channelCount > 1

  local sidechain = self:addGainBiasControl("sidechain")
  local threshold = self:addGainBiasControl("threshold")
  local ratio     = self:addGainBiasControlNoBranch("ratio")
  local gain      = self:addGainBiasControl("gain")
  local rise      = self:addGainBiasControlNoBranch("rise")
  local fall      = self:addGainBiasControlNoBranch("fall")

  local op = self:addObject("op", strike.CPR(stereo))
  connect(threshold, "Out", op, "Threshold")
  connect(ratio,     "Out", op, "Ratio")
  connect(rise,      "Out", op, "Rise")
  connect(fall,      "Out", op, "Fall")
  connect(gain,      "Out", op, "Gain")

  for i = 1, channelCount do
    connect(self, "In"..i, op, "In"..i)
    connect(op,   "Out"..i, self, "Out"..i)
  end

  self:addMonoBranch("ratio", ratio, "In", self.objects.op, "Follow")
  self:addMonoBranch("rise", rise, "In", self.objects.op, "EOF")
  self:addMonoBranch("fall", fall, "In", self.objects.op, "EOR")
end

function CPR:onLoadViews()
  return {
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
    },
    threshold = GainBias {
      button        = "threshold",
      description   = "Threshold",
      branch        = self.branches.threshold,
      gainbias      = self.objects.threshold,
      range         = self.objects.thresholdRange,
      biasMap       = Encoder.getMap("[0,1]"),
      gainMap       = Encoder.getMap("[0,1]"),
      biasPrecision = 3,
      initialBias   = 0.5
    },
    ratio = GainBias {
      button        = "ratio",
      description   = "Ratio",
      branch        = self.branches.ratio,
      gainbias      = self.objects.ratio,
      range         = self.objects.ratioRange,
      biasMap       = Encoder.getMap("[0,10]"),
      gainMap       = Encoder.getMap("[-10,10]"),
      biasPrecision = 3,
      initialBias   = 1
    },
    gain = GainBias {
      button        = "gain",
      description   = "Input Gain",
      branch        = self.branches.gain,
      gainbias      = self.objects.gain,
      range         = self.objects.gainRange,
      biasMap       = Encoder.getMap("[-10,10]"),
      gainMap       = Encoder.getMap("[-10,10]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 1
    }
  }, {
    expanded  = { "threshold", "ratio", "rise", "fall", "gain" },
    collapsed = {}
  }
end

return CPR
