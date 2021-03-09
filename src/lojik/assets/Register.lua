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
  local length   = self:addGainBias("length")
  local origin   = self:addComparator("origin",  app.COMPARATOR_GATE, 0)
  local capture  = self:addComparator("capture", app.COMPARATOR_GATE, 0)
  local shift    = self:addComparator("shift",   app.COMPARATOR_GATE, 0)
  local step     = self:addComparator("step",    app.COMPARATOR_GATE, 0)
  local zero     = self:addComparator("zero",    app.COMPARATOR_GATE, 0)
  local zeroAll  = self:addComparator("zeroAll", app.COMPARATOR_GATE, 0)
  local rand     = self:addComparator("rand",    app.COMPARATOR_GATE, 0)
  local randAll  = self:addComparator("randAll", app.COMPARATOR_GATE, 0)
  local randGain = self:addGainBias("randGain")

  local register = self:addObject("register", lojik.Register())
  connect(length,  "Out",  register, "Length")
  connect(origin,  "Out",  register, "Origin")
  connect(capture, "Out",  register, "Capture")
  connect(shift,   "Out",  register, "Shift")
  connect(step,    "Out",  register, "Step")
  connect(zero,    "Out",  register, "Zero")
  connect(zeroAll, "Out",  register, "ZeroAll")
  connect(rand,    "Out",  register, "Randomize")
  connect(randAll, "Out",  register, "RandomizeAll")
  connect(randGain, "Out", register, "RandomizeGain")

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
    expanded  = { "capture", "step", "shift", "origin", "length" },
    origin    = { "origin", "zero", "zeroAll", "rand", "randAll", "randGain" },
    collapsed = { }
  }

  local makeGateView = self:makeGateViewF(objects, branches)

  local controls = { }

  controls.origin  = makeGateView("origin", "Origin")
  controls.capture = makeGateView("capture", "Capture")
  controls.shift   = makeGateView("shift", "Shift")
  controls.step    = makeGateView("step", "Step")
  controls.zero    = makeGateView("zero", "Zero")
  controls.zeroAll = makeGateView("zeroAll", "Zero All")
  controls.rand    = makeGateView("rand", "Randomize")
  controls.randAll = makeGateView("randAll", "Randomize All")

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

  controls.randGain = GainBias {
    button        = "randGain",
    description   = "Randomize Gain",
    branch        = branches.randGain,
    gainbias      = objects.randGain,
    range         = objects.randGainRange,
    biasMap       = Encoder.getMap("[0,1]"),
    biasUnits     = app.unitNone,
    biasPrecision = 2,
    initialBias   = 1
  }

  return controls, views
end

return Register
