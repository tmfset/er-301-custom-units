local app = app
local strike = require "strike.libstrike"
local Class = require "Base.Class"
local Encoder = require "Encoder"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"
local OutputScope = require "Unit.ViewControl.OutputScope"
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

function Arc.addGainBiasControl(self, name)
  local gb    = self:addObject(name, app.GainBias());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(gb, "Out", range, "In")
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

function Arc.expMap(min, max, resolution, degree)
  local map = app.LUTDialMap(1)
  map:add(min)
  for x = 1, resolution do
    local v = x / resolution
    local e = math.exp(v * math.log(max + degree) + (1 - v) * math.log(min + degree)) - degree
    --local e = math.log(v * math.exp(max + degree) + (1 - v) * math.exp(min + degree)) + degree
    map:add(e)
  end
  map:add(max)
  return map
end

function Arc:onLoadGraph(channelCount)
  local stereo = channelCount > 1

  local rise     = self:addGainBiasControl("rise")
  local fall     = self:addGainBiasControl("fall")
  local bendUp   = self:addGainBiasControl("bendUp")
  local bendDown = self:addGainBiasControl("bendDown")
  local loop     = self:addComparatorControl("loop", app.COMPARATOR_TOGGLE)
  local height   = self:addGainBiasControl("height")

  for i = 1, channelCount do
    local op = self:addObject("op"..i, strike.ADEnvelope())
    connect(self,     "In"..i, op, "In")
    connect(rise,     "Out",   op, "Rise")
    connect(fall,     "Out",   op, "Fall")
    connect(bendUp,   "Out", op, "Bend Up")
    connect(bendDown, "Out", op, "Bend Down")
    connect(loop,     "Out",   op, "Loop")
    connect(height,   "Out",   op, "Height")
    connect(op,       "Out", self, "Out"..i)
  end
end

function Arc:onLoadViews()
  return {
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
    loop = Gate {
      button = "loop",
      description = "Loop",
      branch = self.branches.loop,
      comparator = self.objects.loop
    },
    bendUp = GainBias {
      button        = "rBend",
      description   = "Rise Bend",
      branch        = self.branches.bendUp,
      gainbias      = self.objects.bendUp,
      range         = self.objects.bendUpRange,
      biasMap       = Encoder.getMap("[-1,1]"),
      gainMap       = Encoder.getMap("[-1,1]"),
      biasPrecision = 3,
      initialBias   = 0
    },
    bendDown = GainBias {
      button        = "fBend",
      description   = "Fall Bend",
      branch        = self.branches.bendDown,
      gainbias      = self.objects.bendDown,
      range         = self.objects.bendDownRange,
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
      biasMap       = Encoder.getMap("[-1,1]"),
      gainMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 1
    }
  }, {
    expanded  = { "rise", "bendUp", "fall", "bendDown", "loop", "height" },
    collapsed = {}
  }
end

return Arc
