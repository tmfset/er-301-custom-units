local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local Encoder = require "Encoder"
local Gate = require "Unit.ViewControl.Gate"
local GainBias = require "Unit.ViewControl.GainBias"

local Register = Class {}
Register:include(Unit)

function Register:init(args)
  args.title = "Register"
  args.mnemonic = "R"
  Unit.init(self, args)
end

function Register:onLoadGraph(channelCount)
  if channelCount == 2 then
    self:loadStereoGraph()
  else
    self:loadMonoGraph()
  end
end

function Register:addGainBias(name)
  local gb    = self:addObject(name, app.GainBias());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(gb, "Out", range, "In")
  self:addMonoBranch(name, gb, "In", gb, "Out")
  return gb;
end

function Register:addComparator(name, mode, default)
  local gate = self:addObject(name, app.Comparator())
  gate:setMode(mode)
  self:addMonoBranch(name, gate, "In", gate, "Out")
  if default then
    gate:setOptionValue("State", default)
  end
  return gate
end

function Register:loadMonoGraph()
  local length  = self:addGainBias("length")
  local clock   = self:addComparator("clock",   app.COMPARATOR_GATE, 0)
  local capture = self:addComparator("capture", app.COMPARATOR_TOGGLE, 0)
  local shift   = self:addComparator("shift",   app.COMPARATOR_TOGGLE, 0)
  local reset   = self:addComparator("reset",   app.COMPARATOR_TOGGLE, 0)
  local scatter = self:addComparator("scatter", app.COMPARATOR_TOGGLE, 0)
  local gain    = self:addGainBias("gain")

  local register = self:addObject("register", lojik.Register())
  connect(length,  "Out", register, "Length")
  connect(clock,   "Out", register, "Clock")
  connect(capture, "Out", register, "Capture")
  connect(shift,   "Out", register, "Shift")
  connect(reset,   "Out", register, "Reset")
  connect(scatter, "Out", register, "Scatter")
  connect(gain,    "Out", register, "Gain")

  connect(self, "In1", register, "In")
  connect(register, "Out", self, "Out1")
end

function Register:loadStereoGraph()
  self:loadMonoGraph()
  connect(self.objects.register, "Out", self, "Out2")
end

function Register:makeGateViewF(objects, branches)
  return function (name, description)
    return Gate {
      button      = name,
      description = description,
      branch      = branches[name],
      comparator  = objects[name]
    }
  end
end

function Register:onLoadViews(objects, branches)
  local views = {
    expanded  = { "clock", "capture", "shift", "reset", "scatter", "gain", "length" },
    collapsed = { }
  }

  local makeGateView = self:makeGateViewF(objects, branches)

  local controls = { }

  controls.clock   = makeGateView("clock", "Clock")
  controls.capture = makeGateView("capture", "Capture")
  controls.shift   = makeGateView("shift", "Shift")
  controls.reset   = makeGateView("reset", "Reset")
  controls.scatter = makeGateView("scatter", "Scatter")

  local intMap = function (min, max)
    local map = app.LinearDialMap(min,max)
    map:setSteps(5, 1, 0.25, 0.25)
    map:setRounding(1)
    return map
  end

  controls.length = GainBias {
    button        = "length",
    description   = "Length",
    branch        = branches.length,
    gainbias      = objects.length,
    range         = objects.lengthRange,
    gainMap       = intMap(-128, 128),
    biasMap       = intMap(1, 128),
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
    biasPrecision = 2,
    initialBias   = 1
  }

  return controls, views
end

return Register
