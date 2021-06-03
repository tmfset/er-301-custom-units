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

local Arc = Class {}
Arc:include(Unit)

function Arc:init(args)
  args.title = "Arc"
  args.mnemonic = "ad"
  Unit.init(self, args)
end

function Arc.addComparatorControl(self, name, mode, default)
  local gate = self:addObject(name, app.Comparator())
  gate:setMode(mode)
  self:addMonoBranch(name, gate, "In", gate, "Out")
  if default then
    gate:setOptionValue("State", default)
  end
  return gate
end

function Arc.addGainBiasControlNoBranch(self, name)
  local gb    = self:addObject(name, app.GainBias());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(gb, "Out", range, "In")
  return gb;
end

function Arc.addGainBiasControl(self, name)
  local gb = self:addGainBiasControlNoBranch(name);
  self:addMonoBranch(name, gb, "In", gb, "Out")
  return gb;
end

function Arc.addConstantOffsetControl(self, name)
  local co    = self:addObject(name, app.ConstantOffset());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(co, "Out", range, "In")
  self:addMonoBranch(name, co, "In", co, "Out")
  return co;
end

function Arc.linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

function Arc:onShowMenu(objects, branches)
  return {
    mode = OptionControl {
      description = "Bend Mode",
      option      = self.objects.op1:getOption("Bend Mode"),
      choices     = { "hump", "fin" }
    },
    sense = OptionControl {
      description = "Input Sense",
      option      = self.objects.op1:getOption("Sense"),
      choices     = { "low", "high" }
    }
  }, { "mode", "sense" }
end

function Arc:onLoadGraph(channelCount)
  local stereo = channelCount > 1

  local rise     = self:addGainBiasControlNoBranch("rise")
  local fall     = self:addGainBiasControlNoBranch("fall")
  local bend     = self:addGainBiasControl("bend")
  local loop     = self:addComparatorControl("loop", app.COMPARATOR_TOGGLE)
  local height   = self:addGainBiasControl("height")

  for i = 1, channelCount do
    local op = self:addObject("op"..i, strike.Arc())
    connect(rise,   "Out",   op, "Rise")
    connect(fall,   "Out",   op, "Fall")
    connect(bend,   "Out",   op, "Bend")
    connect(loop,   "Out",   op, "Loop")
    connect(height, "Out",   op, "Height")

    if i > 1 then
      tie(op, "Bend Mode", self.objects.op1, "Bend Mode")
    end

    connect(self,   "In"..i, op, "In")
    connect(op,     "Out", self, "Out"..i)
  end

  self:addMonoBranch("rise", rise, "In", self.objects.op1, "EOF")
  self:addMonoBranch("fall", fall, "In", self.objects.op1, "EOR")
end

function Arc:onLoadViews()
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
    loop = Gate {
      button      = "loop",
      description = "Loop",
      branch      = self.branches.loop,
      comparator  = self.objects.loop
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
      initialBias   = -0.5
    },
    height = GainBias {
      button        = "height",
      description   = "Height",
      branch        = self.branches.height,
      gainbias      = self.objects.height,
      range         = self.objects.heightRange,
      biasMap       = Encoder.getMap("[-1,1]"),
      gainMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 1
    }
  }, {
    expanded  = { "loop", "rise", "fall", "bend", "height" },
    collapsed = {}
  }
end

return Arc
