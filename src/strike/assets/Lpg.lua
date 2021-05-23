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

local Lpg = Class {}
Lpg:include(Unit)

function Lpg:init(args)
  args.title = "Lpg"
  args.mnemonic = "lpg"
  Unit.init(self, args)
end

function Lpg.addComparatorControl(self, name, mode, default)
  local gate = self:addObject(name, app.Comparator())
  gate:setMode(mode)
  self:addMonoBranch(name, gate, "In", gate, "Out")
  if default then
    gate:setOptionValue("State", default)
  end
  return gate
end

function Lpg.addGainBiasControl(self, name)
  local gb    = self:addObject(name, app.GainBias());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(gb, "Out", range, "In")
  self:addMonoBranch(name, gb, "In", gb, "Out")
  return gb;
end

function Lpg.addConstantOffsetControl(self, name)
  local co    = self:addObject(name, app.ConstantOffset());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(co, "Out", range, "In")
  self:addMonoBranch(name, co, "In", co, "Out")
  return co;
end

function Lpg.linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

function Lpg:onLoadGraph(channelCount)
  local stereo = channelCount > 1

  local trig   = self:addComparatorControl("trig", app.COMPARATOR_TRIGGER_ON_RISE)
  local loop   = self:addComparatorControl("loop", app.COMPARATOR_TOGGLE)
  local rise   = self:addGainBiasControl("rise")
  local fall   = self:addGainBiasControl("fall")
  local bend   = self:addGainBiasControl("bend")
  local height = self:addGainBiasControl("height")

  local op = self:addObject("op", strike.LowPassGate(stereo))
  connect(trig,   "Out", op, "Trig")
  connect(loop,   "Out", op, "Loop")
  connect(rise,   "Out", op, "Rise")
  connect(fall,   "Out", op, "Fall")
  connect(bend,   "Out", op, "Bend")
  connect(height, "Out", op, "Height")

  for i = 1, channelCount do
    connect(self, "In"..i, op, "In"..i)
    connect(op, "Out"..i, self, "Out"..i)
  end
end

function Lpg:onShowMenu(objects, branches)
  return {
    bendMode = OptionControl {
      description = "Bend Mode",
      option      = self.objects.op:getOption("Bend Mode"),
      choices     = { "together", "inverted" }
    }
  }, { "bendMode" }
end

function Lpg:onLoadViews()
  return {
    trig = Gate {
      button = "trig",
      description = "Trigger",
      branch = self.branches.trig,
      comparator = self.objects.trig
    },
    loop = Gate {
      button = "loop",
      description = "Loop",
      branch = self.branches.loop,
      comparator = self.objects.loop
    },
    rise = GainBias {
      button = "rise",
      branch = self.branches.rise,
      description = "Rise Time",
      gainbias = self.objects.rise,
      range = self.objects.riseRange,
      biasMap = self.linMap(0, 10, 1, 0.1, 0.01, 0.001),
      biasUnits = app.unitSecs,
      initialBias = 0.050
    },
    fall = GainBias {
      button = "fall",
      branch = self.branches.fall,
      description = "Fall Time",
      gainbias = self.objects.fall,
      range = self.objects.fallRange,
      biasMap = self.linMap(0, 10, 1, 0.1, 0.01, 0.001),
      biasUnits = app.unitSecs,
      initialBias = 0.200
    },
    bend = GainBias {
      button        = "bend",
      description   = "Bend",
      branch        = self.branches.bend,
      gainbias      = self.objects.bend,
      range         = self.objects.bendRange,
      biasMap       = Encoder.getMap("[-1,1]"),
      gainMap       = Encoder.getMap("[-1,1]"),
      biasPrecision = 3,
      initialBias   = 0
    },
    height = GainBias {
      button        = "height",
      description   = "Height",
      branch        = self.branches.height,
      gainbias      = self.objects.height,
      range         = self.objects.heightRange,
      biasMap       = Encoder.getMap("[0,1]"),
      gainMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 1
    }
  }, {
    expanded  = { "trig", "loop", "rise", "fall", "bend", "height" },
    collapsed = {}
  }
end

return Lpg
