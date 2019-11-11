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

local MultiVoice = Class{}
MultiVoice:include(Unit)

function MultiVoice:init(args)
  self.roundRobin = true
  self.voiceCount = args.voiceCount or app.error("%s.init: voiceCount is missing.", self)
  args.version = 1
  Unit.init(self, args)
end

function MultiVoice:onLoadGraph(channelCount)
  local voiceMixer = self:createObject("Mixer", "voiceMixer", self.voiceCount)
  connect(voiceMixer, "Out", self, "Out1")

  local gate = self:createObject("Comparator", "gate")
  gate:setGateMode()
  self:createMonoBranch("gate", gate, "In", gate, "Out")

  local tune = self:createObject("ConstantOffset", "tune")
  local tuneRange = self:createObject("MinMax", "tuneRange")
  connect(tune, "Out", tuneRange, "In")
  self:createMonoBranch("tune", tune, "In", tune, "Out")

  local switch = self:createObject("Comparator", "switch")
  switch:setTriggerOnFallMode()
  connect(gate, "Out", switch, "In")

  local rrGate = self:createObject("RoundRobin", "rrGate", self.voiceCount)
  connect(switch, "Out", rrGate, "Trigger")
  connect(gate, "Out", rrGate, "Signal")
  rrGate:compile()

  local rrTune = self:createObject("RoundRobin", "rrTune", self.voiceCount)
  connect(switch, "Out", rrTune, "Trigger")
  connect(tune, "Out", rrTune, "Signal")
  rrTune:compile()

  local f0 = self:createObject("GainBias", "f0")
  local f0Range = self:createObject("MinMax", "f0Range")
  connect(f0, "Out", f0Range, "In")
  self:createMonoBranch("f0", f0, "In", f0, "Out")

  local detune = self:createObject("ConstantOffset", "detune")
  local detuneRange = self:createObject("MinMax", "detuneRange")
  connect(detune, "Out", detuneRange, "In")
  self:createMonoBranch("detune", detune, "In", detune, "Out")

  local cutoff = self:createObject("GainBias", "cutoff")
  local cutoffRange = self:createObject("MinMax", "cutoffRange")
  connect(cutoff, "Out", cutoffRange, "In")
  self:createMonoBranch("cutoff", cutoff, "In", cutoff, "Out")

  local res = self:createObject("GainBias", "res")
  local resRange = self:createObject("MinMax", "resRange")
  local clipper = self:createObject("Clipper", "clipper")
  clipper:setMaximum(0.999)
  clipper:setMinimum(0)
  connect(res, "Out", clipper, "In")
  connect(clipper, "Out", resRange, "In")
  self:createMonoBranch("Q", res, "In", res, "Out")

  local fenv = self:createObject("GainBias", "fenv")
  local fenvRange = self:createObject("MinMax", "fenvRange")
  connect(fenv, "Out", fenvRange, "In")
  self:createMonoBranch("fenv", fenv, "In", fenv, "Out")

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

  for i = 1, self.voiceCount do
    local vGate = self:createObject("Comparator", "gate"..i)
    vGate:setGateMode()

    local vTune = self:createObject("ConstantOffset", "tune"..i)
    local vTuneRange = self:createObject("MinMax", "tuneRange"..i)
    connect(vTune, "Out", vTuneRange, "In")

    if self.roundRobin then
      connect(rrGate, "Out"..i, vGate, "In")
      connect(rrTune, "Out"..i, vTune, "In")
    else
      self:createMonoBranch("gate"..i, vGate, "In", vGate, "Out")
      self:createMonoBranch("tune"..i, vTune, "In", vTune, "Out")
    end

    local oscA = self:createObject("SawtoothOscillator", "oscA"..i)
    local oscB = self:createObject("SawtoothOscillator", "oscB"..i)
    local detuneSum = self:createObject("Sum", "detuneSum"..i)

    connect(vTune, "Out", detuneSum, "Left")
    connect(detune, "Out", detuneSum, "Right")

    connect(f0, "Out", oscA, "Fundamental")
    connect(vTune, "Out", oscA, "V/Oct")
    connect(f0, "Out", oscB, "Fundamental")
    connect(detuneSum, "Out", oscB, "V/Oct")

    local mixer = self:createObject("Mixer", "mixer"..i, 2)
    mixer:hardSet("Gain1", 0.5)
    mixer:hardSet("Gain2", 0.5)

    connect(oscA, "Out", mixer, "In1")
    connect(oscB, "Out", mixer, "In2")

    local adsr = self:createObject("ADSR", "adsr"..i)
    local vca = self:createObject("Multiply", "vca"..i)

    connect(vGate, "Out", adsr, "Gate")
    connect(attack, "Out", adsr, "Attack")
    connect(decay, "Out", adsr, "Decay")
    connect(sustain, "Out", adsr, "Sustain")
    connect(release, "Out", adsr, "Release")

    connect(adsr, "Out", vca, "Left")
    connect(mixer, "Out", vca, "Right")

    local fenvVca = self:createObject("Multiply", "fenvVca"..i)
    local filter = self:createObject("StereoLadderFilter", "filter"..i)

    connect(adsr, "Out", fenvVca, "Left")
    connect(fenv, "Out", fenvVca, "Right")
    connect(fenvVca, "Out", filter, "V/Oct")
    connect(cutoff, "Out", filter, "Fundamental")
    connect(clipper, "Out", filter, "Resonance")

    connect(vca, "Out", filter, "Left In")
    connect(filter, "Left Out", voiceMixer, "In"..i)
    voiceMixer:hardSet("Gain"..i, 1 / self.voiceCount)
  end
end

function MultiVoice:onLoadViews(objects, branches)
  local controls = {}
  local views = { expanded = {}, collapsed = {} }
  local controlCount = 1

  controls.scope = OutputScope {
    monitor = self,
    width = 4 * ply,
  }

  if self.roundRobin then
    controls.gate = Gate {
      button = "gate",
      description = "Round Robin Gate",
      branch = branches.gate,
      comparator = objects.gate
    }
    views.expanded[controlCount] = "gate"
    controlCount = controlCount + 1

    controls.tune = Pitch {
      button = "V/oct",
      description = "Round Robin V/oct",
      branch = branches.tune,
      offset = objects.tune,
      range = objects.tuneRange
    }
    views.expanded[controlCount] = "tune"
    controlCount = controlCount + 1
  else
    for i = 1, self.voiceCount do
      controls["gate"..i] = Gate {
        button = "gate"..i,
        description = "Gate "..i,
        branch = branches["gate"..i],
        comparator = objects["gate"..i]
      }
      views.expanded[controlCount] = "gate"..i
      controlCount = controlCount + 1

      controls["tune"..i] = Pitch {
        button = "V/oct"..i,
        description = "V/oct "..i,
        branch = branches["tune"..i],
        offset = objects["tune"..i],
        range = objects["tuneRange"..i]
      }
      views.expanded[controlCount] = "tune"..i
      controlCount = controlCount + 1
    end
  end

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

  controls.detune = Pitch {
    button = "detune",
    description = "Detune",
    branch = branches.detune,
    offset = objects.detune,
    range = objects.detuneRange
  }
  views.expanded[controlCount] = "detune"
  controlCount = controlCount + 1

  controls.cutoff = GainBias {
    button = "cutoff",
    branch = branches.cutoff,
    description = "Filter Cutoff",
    gainbias = objects.cutoff,
    range = objects.cutoffRange,
    biasMap = Encoder.getMap("filterFreq"),
    biasUnits = app.unitHertz,
    initialBias = 440,
    gainMap = Encoder.getMap("freqGain"),
    scaling = app.octaveScaling
  }
  views.expanded[controlCount] = "cutoff"
  controlCount = controlCount + 1

  controls.resonance = GainBias {
    button = "Q",
    branch = branches.Q,
    description = "Resonance",
    gainbias = objects.res,
    range = objects.resRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 0.25,
    gainMap = Encoder.getMap("[-10,10]")
  }
  views.expanded[controlCount] = "resonance"
  controlCount = controlCount + 1

  controls.fenv = GainBias {
    button = "fenv",
    description = "Filter Env Amount",
    branch = branches.fenv,
    gainbias = objects.fenv,
    range = objects.fenvRange,
    biasMap = Encoder.getMap("[-1,1]"),
    initialBias = 0.5
  }
  views.expanded[controlCount] = "fenv"
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

return MultiVoice
