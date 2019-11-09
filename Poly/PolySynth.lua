-- luacheck: globals app connect
local app = app
local Class = require "Base.Class"
local Unit = require "Unit"
local Pitch = require "Unit.ViewControl.Pitch"
local Fader = require "Unit.ViewControl.Fader"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"
local OutputScope = require "Unit.ViewControl.OutputScope"
local Encoder = require "Encoder"
local ply = app.SECTION_PLY

local PolySynth = Class{}
PolySynth:include(Unit)

function PolySynth:init(args)
  self.voiceCount = args.voiceCount or app.error("%s.init: voiceCount is missing.", self)
  args.title = "Poly Synth "..self.voiceCount
  args.mnemonic = "PS%s"..self.voiceCount
  args.version = 0
  Unit.init(self, args)
end

function PolySynth:onLoadGraph(channelCount)
  local f0 = self:createObject("GainBias", "f0")
  local f0Range = self:createObject("MinMax", "f0Range")
  connect(f0, "Out", f0Range, "In")
  self:createMonoBranch("f0", f0, "In", f0, "Out")

  local attack = self:createObject("GainBias", "attack")
  local attackRange = self:createObject("MinMax", "attackRange")
  connect(attack, "Out", attackRange, "In")
  self:createMonoBranch("attack", attack, "In", attack, "Out")

  local decay = self:createObject("GainBias", "decay")
  local decayRange = self:createObject("MinMax", "decayRange")
  connect(decay, "Out", decayRange, "In")
  self:createMonoBranch("decay", decay, "In", decay, "Out")

  local sustain = self:createObject("GainBias", "sustain")
  local sustainRange = self:createObject("MinMax", "sustainRange")
  connect(sustain, "Out", sustainRange, "In")
  self:createMonoBranch("sustain", sustain, "In", sustain, "Out")

  local release = self:createObject("GainBias", "release")
  local releaseRange = self:createObject("MinMax", "releaseRange")
  connect(release, "Out", releaseRange, "In")
  self:createMonoBranch("release", release, "In", release, "Out")

  local sums = {}
  for i = 1, self.voiceCount do
    local tune = self:createObject("ConstantOffset", "tune"..i)
    local tuneRange = self:createObject("MinMax", "tuneRange"..i)
    connect(tune, "Out", tuneRange, "In")
    self:createMonoBranch("tune"..i, tune, "In", tune, "Out")

    local osc = self:createObject("SineOscillator", "osc"..i)
    connect(tune, "Out", osc, "V/Oct")
    connect(f0, "Out", osc, "Fundamental")

    local adsr = self:createObject("ADSR", "adsr"..i)
    local vca = self:createObject("Multiply", "vca"..i)
    local sum = self.createObject("Sum", "sum"..i)
    connect(osc, "Out", vca, "Right")
    connect(adsr, "Out", vca, "Left")
    connect(vca, "Out", sum, "Left")
    sums[i] = sum

    local gate = self:createObject("Comparator", "gate"..i)
    gate:setGateMode()
    self:createMonoBranch("gate"..i, gate, "In", gate, "Out")

    connect(gate, "Out", adsr, "Gate")
    connect(attack, "Out", adsr, "Attack")
    connect(decay, "Out", adsr, "Decay")
    connect(sustain, "Out", adsr, "Sustain")
    connect(release, "Out", adsr, "Release")
  end

  for i = 2, self.voiceCount do
    connect(sums[i - 1], "Out", sums[i], "Right")
  end
  connect(sums[self.voiceCount], "Out", self, "Out1")
end

function PolySynth:onLoadViews(objects, branches)
  local controls = {}
  local views = { expanded = {}, collapsed = {} }

  controls.scope = OutputScope {
    monitor = self,
    width = 4 * ply,
  }

  for i = 1, self.voiceCount do
    controls["gate"..i] = Gate {
      button = "gate"..i,
      description = "Gate "..i,
      branch = branches["gate"..i],
      comparator = objects["gate"..i]
    }

    controls["tune"..i] = Pitch {
      button = "V/oct"..i,
      description = "V/oct "..i,
      branch = branches["tune"..i],
      offset = objects["tune"..i],
      range = objects["tuneRange"..i]
    }

    views.expanded[((i - 1) * 2) + 1] = "gate"..i
    views.expanded[((i - 1) * 2) + 2] = "tune"..i
  end

  local controlCount = (self.voiceCount * 2) + 1

  controls.freq = GainBias {
    button = "f0",
    description = "Fundamental",
    branch = branches.f0,
    gainbias = objects.f0,
    range = objects.f0Range,
    biasMap = Encoder.getMap("oscFreq"),
    biasUnits = app.unitHertz,
    initialBias = 27.5,
    gainMap = Encoder.getMap("freqGain"),
    scaling = app.octaveScaling
  }
  views.expanded[controlCount] = "freq"
  controlCount = controlCount + 1

  controls.attack = GainBias {
    button = "A",
    branch = branches.attack,
    description = "Attack",
    gainbias = objects.attack,
    range = objects.attackRange,
    biasMap = Encoder.getMap("ADSR"),
    biasUnits = app.unitSecs,
    initialBias = 0.050
  }
  views.expanded[controlCount] = "attack"
  controlCount = controlCount + 1

  controls.decay = GainBias {
    button = "D",
    branch = branches.decay,
    description = "Decay",
    gainbias = objects.decay,
    range = objects.decayRange,
    biasMap = Encoder.getMap("ADSR"),
    biasUnits = app.unitSecs,
    initialBias = 0.050
  }
  views.expanded[controlCount] = "decay"
  controlCount = controlCount + 1

  controls.sustain = GainBias {
    button = "S",
    branch = branches.sustain,
    description = "Sustain",
    gainbias = objects.sustain,
    range = objects.sustainRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 1
  }
  views.expanded[controlCount] = "sustain"
  controlCount = controlCount + 1

  controls.release = GainBias {
    button = "R",
    branch = branches.release,
    description = "Release",
    gainbias = objects.release,
    range = objects.releaseRange,
    biasMap = Encoder.getMap("ADSR"),
    biasUnits = app.unitSecs,
    initialBias = 0.100
  }
  views.expanded[controlCount] = "release"

  return controls, views
end

return PolySynth
