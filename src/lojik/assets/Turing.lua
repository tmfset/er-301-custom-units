local app = app
local lojik = require "lojik.liblojik"
local core = require "core.libcore"
local Class = require "Base.Class"
local Unit = require "Unit"
local Encoder = require "Encoder"
local Gate = require "Unit.ViewControl.Gate"
local GainBias = require "Unit.ViewControl.GainBias"
local PitchCircle = require "core.Quantizer.PitchCircle"
local ply = app.SECTION_PLY

local Turing = Class {}
Turing:include(Unit)

function Turing:init(args)
  args.title = "Turing"
  args.mnemonic = "TM"
  Unit.init(self, args)
end

function Turing:onLoadGraph(channelCount)
  if channelCount == 2 then
    self:loadStereoGraph()
  else
    self:loadMonoGraph()
  end
end

function Turing:addGainBias(name)
  local gb    = self:addObject(name, app.GainBias());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(gb, "Out", range, "In")
  self:addMonoBranch(name, gb, "In", gb, "Out")
  return gb;
end

function Turing:addComparator(name, mode, default)
  local gate = self:addObject(name, app.Comparator())
  gate:setMode(mode)
  self:addMonoBranch(name, gate, "In", gate, "Out")
  if default then
    gate:setOptionValue("State", default)
  end
  return gate
end

function Turing:loadMonoGraph()
  local step    = self:addComparator("step",  app.COMPARATOR_TRIGGER_ON_RISE, 0)
  local write   = self:addComparator("write", app.COMPARATOR_TOGGLE, 0)
  local length  = self:addGainBias("length")

  local shift   = self:addComparator("shift", app.COMPARATOR_GATE, 0)
  local reset   = self:addComparator("reset", app.COMPARATOR_GATE, 0)

  local gain    = self:addGainBias("gain")
  local scatter = self:addComparator("scatter", app.COMPARATOR_TOGGLE, 1)

  local register = self:addObject("register", lojik.Register())
  connect(self,    "In1", register, "In")
  connect(step,    "Out", register, "Clock")
  connect(write,   "Out", register, "Capture")
  connect(length,  "Out", register, "Length")
  connect(shift,   "Out", register, "Shift")
  connect(reset,   "Out", register, "Reset")
  connect(gain,    "Out", register, "Gain")
  connect(scatter, "Out", register, "Scatter")

  local quantizer = self:addObject("quantizer", core.ScaleQuantizer())
  connect(register,  "Out", quantizer, "In")
  connect(quantizer, "Out", self,      "Out1")
end

function Turing:loadStereoGraph()
  self:loadMonoGraph()
  connect(self.objects.quantizer, "Out", self, "Out2")
end

function Turing:makeGateViewF(objects, branches)
  return function (name, description)
    return Gate {
      button      = name,
      description = description,
      branch      = branches[name],
      comparator  = objects[name]
    }
  end
end

function Turing:onLoadViews(objects, branches)
  local views = {
    expanded  = { "step", "write", "length", "scale" },
    step      = { "step", "shift", "reset" },
    write     = { "write", "gain", "scatter" },
    collapsed = { "scale" }
  }

  local makeGateView = self:makeGateViewF(objects, branches)

  local controls = { }

  controls.step    = makeGateView("step", "Advance")
  controls.write   = makeGateView("write", "Enable Write")
  controls.shift   = makeGateView("shift", "Enable Shift")
  controls.reset   = makeGateView("reset", "Enable Reset")
  controls.scatter = makeGateView("scatter", "Enable Scatter")

  local intMap = function (min, max)
    local map = app.LinearDialMap(min,max)
    map:setSteps(1, 1, 1, 1)
    map:setRounding(1)
    return map
  end

  controls.length = GainBias {
    button        = "length",
    description   = "Length",
    branch        = branches.length,
    gainbias      = objects.length,
    range         = objects.lengthRange,
    gainMap       = intMap(-16, 16),
    biasMap       = intMap(1, 16),
    biasPrecision = 0,
    initialBias   = 4
  }

  controls.gain = GainBias {
    button        = "gain",
    description   = "Input Gain",
    branch        = branches.gain,
    gainbias      = objects.gain,
    range         = objects.gainRange,
    biasMap       = Encoder.getMap("[0,1]"),
    biasUnits     = app.unitNone,
    biasPrecision = 3,
    initialBias   = 0.125
  }

  controls.scale = PitchCircle {
    name      = "scale",
    width     = 2 * ply,
    quantizer = objects.quantizer
  }

  return controls, views
end

return Turing
