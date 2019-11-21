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
  self.voiceCount = args.voiceCount or app.error("%s.init: voiceCount is missing.", self)
  args.version = 1
  Unit.init(self, args)
end

function MultiVoice:createControls()
  return {
    f0           = self:createObject("GainBias", "f0"),
    f0Range      = self:createObject("MinMax", "f0Range"),
    detune       = self:createObject("ConstantOffset", "detune"),
    detuneRange  = self:createObject("MinMax", "detuneRange"),
    cutoff       = self:createObject("GainBias", "cutoff"),
    cutoffRange  = self:createObject("MinMax", "cutoffRange"),
    res          = self:createObject("GainBias", "res"),
    resRange     = self:createObject("MinMax", "resRange"),
    clipper      = self:createObject("Clipper", "clipper"),
    fenv         = self:createObject("GainBias", "fenv"),
    fenvRange    = self:createObject("MinMax", "fenvRange"),
    attack       = self:createObject("GainBias", "attack"),
    attackRange  = self:createObject("MinMax", "attackRange"),
    decay        = self:createObject("GainBias", "decay"),
    decayRange   = self:createObject("MinMax", "decayRange"),
    sustain      = self:createObject("GainBias", "sustain"),
    sustainRange = self:createObject("MinMax", "sustainRange"),
    release      = self:createObject("GainBias", "release"),
    releaseRange = self:createObject("MinMax", "releaseRange")
  }
end

function MultiVoice:connectControls(controls)
  connect(controls.f0, "Out", controls.f0Range, "In")
  connect(controls.detune, "Out", controls.detuneRange, "In")
  connect(controls.cutoff, "Out", controls.cutoffRange, "In")
  connect(controls.res, "Out", controls.clipper, "In")
  connect(controls.clipper, "Out", controls.resRange, "In")
  connect(controls.fenv, "Out", controls.fenvRange, "In")
  connect(controls.attack, "Out", controls.attackRange, "In")
  connect(controls.decay, "Out", controls.decayRange, "In")
  connect(controls.sustain, "Out", controls.sustainRange, "In")
  connect(controls.release, "Out", controls.releaseRange, "In")

  controls.clipper:setMaximum(0.999)
  controls.clipper:setMinimum(0)

  self:createMonoBranch("f0", controls.f0, "In", controls.f0, "Out")
  self:createMonoBranch("detune", controls.detune, "In", controls.detune, "Out")
  self:createMonoBranch("cutoff", controls.cutoff, "In", controls.cutoff, "Out")
  self:createMonoBranch("Q", controls.res, "In", controls.res, "Out")
  self:createMonoBranch("fenv", controls.fenv, "In", controls.fenv, "Out")
  self:createMonoBranch("attack", controls.attack, "In", controls.attack, "Out")
  self:createMonoBranch("decay", controls.decay, "In", controls.decay, "Out")
  self:createMonoBranch("sustain", controls.sustain, "In", controls.sustain, "Out")
  self:createMonoBranch("release", controls.release, "In", controls.release, "Out")
end

function MultiVoice:createVoice(i)
  return {
    index     = i,
    tune      = self:createObject("ConstantOffset", "tune"..i),
    tuneRange = self:createObject("MinMax", "tuneRange"..i),
    oscA      = self:createObject("SawtoothOscillator", "oscA"..i),
    oscB      = self:createObject("SawtoothOscillator", "oscB"..i),
    detuneSum = self:createObject("Sum", "detuneSum"..i),
    mixer     = self:createObject("Mixer", "mixer"..i, 2),
    gate      = self:createObject("Comparator", "gate"..i),
    adsr      = self:createObject("ADSR", "adsr"..i),
    vca       = self:createObject("Multiply", "vca"..i),
    fenvVca   = self:createObject("Multiply", "fenvVca"..i),
    filter    = self:createObject("StereoLadderFilter", "filter"..i)
  }
end

function MultiVoice:connectVoice(voice, controls)
  connect(controls.f0, "Out", voice.oscA, "Fundamental")
  connect(controls.f0, "Out", voice.oscB, "Fundamental")
  connect(voice.tune, "Out", voice.tuneRange, "In")
  connect(voice.tune, "Out", voice.detuneSum, "Left")
  connect(controls.detune, "Out", voice.detuneSum, "Right")
  connect(voice.tune, "Out", voice.oscA, "V/Oct")
  connect(voice.detuneSum, "Out", voice.oscB, "V/Oct")

  connect(voice.oscA, "Out", voice.mixer, "In1")
  connect(voice.oscB, "Out", voice.mixer, "In2")
  voice.mixer:hardSet("Gain1", 0.5)
  voice.mixer:hardSet("Gain2", 0.5)

  connect(voice.gate, "Out", voice.adsr, "Gate")
  voice.gate:setGateMode()

  connect(voice.adsr, "Out", voice.fenvVca, "Left")
  connect(voice.adsr, "Out", voice.vca, "Left")
  connect(voice.mixer, "Out", voice.vca, "Right")
  connect(controls.attack, "Out", voice.adsr, "Attack")
  connect(controls.decay, "Out", voice.adsr, "Decay")
  connect(controls.sustain, "Out", voice.adsr, "Sustain")
  connect(controls.release, "Out", voice.adsr, "Release")

  connect(controls.fenv, "Out", voice.fenvVca, "Right")
  connect(voice.fenvVca, "Out", voice.filter, "V/Oct")
  connect(controls.cutoff, "Out", voice.filter, "Fundamental")
  connect(controls.clipper, "Out", voice.filter, "Resonance")
  connect(voice.vca, "Out", voice.filter, "Left In")

  self:createMonoBranch("gate"..voice.index, voice.gate, "In", voice.gate, "Out")
  self:createMonoBranch("tune"..voice.index, voice.tune, "In", voice.tune, "Out")

  return voice.filter, "Left Out"
end

function MultiVoice:onLoadGraph(channelCount)
  local voiceMixer = self:createObject("Mixer", "voiceMixer", self.voiceCount)
  connect(voiceMixer, "Out", self, "Out1")

  local controls = self:createControls()
  self:connectControls(controls)

  for i = 1, self.voiceCount do
    local voice = self:createVoice(i)
    local out, key = self:connectVoice(voice, controls)

    connect(out, key, voiceMixer, "In"..i)
    voiceMixer:hardSet("Gain"..i, 1 / self.voiceCount)
  end
end

function MultiVoice:setVoiceViews(i, objects, branches, controls, views)
  controls["gate"..i] = Gate {
    button      = "gate"..i,
    description = "Gate "..i,
    branch      = branches["gate"..i],
    comparator  = objects["gate"..i]
  }
  views.expanded[#views.expanded + 1] = "gate"..i

  controls["tune"..i] = Pitch {
    button      = "V/oct"..i,
    description = "V/oct "..i,
    branch      = branches["tune"..i],
    offset      = objects["tune"..i],
    range       = objects["tuneRange"..i]
  }
  views.expanded[#views.expanded + 1] = "tune"..i
end

function MultiVoice:setPitchViews(objects, branches, controls, views)
  controls.freq = GainBias {
    button      = "f0",
    description = "Fundamental",
    branch      = branches.f0,
    gainbias    = objects.f0,
    range       = objects.f0Range,
    biasMap     = Encoder.getMap("oscFreq"),
    biasUnits   = app.unitHertz,
    initialBias = 27.5,
    gainMap     = Encoder.getMap("freqGain"),
    scaling     = app.octaveScaling
  }
  views.expanded[#views.expanded + 1] = "freq"

  controls.detune = Pitch {
    button      = "detune",
    description = "Detune",
    branch      = branches.detune,
    offset      = objects.detune,
    range       = objects.detuneRange
  }
  views.expanded[#views.expanded + 1] = "detune"
end

function MultiVoice:setFilterViews(objects, branches, controls, views)
  controls.cutoff = GainBias {
    button      = "cutoff",
    branch      = branches.cutoff,
    description = "Filter Cutoff",
    gainbias    = objects.cutoff,
    range       = objects.cutoffRange,
    biasMap     = Encoder.getMap("filterFreq"),
    biasUnits   = app.unitHertz,
    initialBias = 440,
    gainMap     = Encoder.getMap("freqGain"),
    scaling     = app.octaveScaling
  }
  views.expanded[#views.expanded + 1] = "cutoff"

  controls.resonance = GainBias {
    button      = "Q",
    branch      = branches.Q,
    description = "Resonance",
    gainbias    = objects.res,
    range       = objects.resRange,
    biasMap     = Encoder.getMap("unit"),
    biasUnits   = app.unitNone,
    initialBias = 0.25,
    gainMap     = Encoder.getMap("[-10,10]")
  }
  views.expanded[#views.expanded + 1] = "resonance"

  controls.fenv = GainBias {
    button      = "fenv",
    description = "Filter Env Amount",
    branch      = branches.fenv,
    gainbias    = objects.fenv,
    range       = objects.fenvRange,
    biasMap     = Encoder.getMap("[-1,1]"),
    initialBias = 0.5
  }
  views.expanded[#views.expanded + 1] = "fenv"
end

function MultiVoice:setAdsrViews(objects, branches, controls, views)
  controls.attack = GainBias {
    button      = "A",
    branch      = branches.attack,
    description = "Attack",
    gainbias    = objects.attack,
    range       = objects.attackRange,
    biasMap     = Encoder.getMap("ADSR"),
    biasUnits   = app.unitSecs,
    initialBias = 0.050
  }
  views.expanded[#views.expanded + 1] = "attack"

  controls.decay = GainBias {
    button      = "D",
    branch      = branches.decay,
    description = "Decay",
    gainbias    = objects.decay,
    range       = objects.decayRange,
    biasMap     = Encoder.getMap("ADSR"),
    biasUnits   = app.unitSecs,
    initialBias = 0.050
  }
  views.expanded[#views.expanded + 1] = "decay"

  controls.sustain = GainBias {
    button      = "S",
    branch      = branches.sustain,
    description = "Sustain",
    gainbias    = objects.sustain,
    range       = objects.sustainRange,
    biasMap     = Encoder.getMap("unit"),
    biasUnits   = app.unitNone,
    initialBias = 1
  }
  views.expanded[#views.expanded + 1] = "sustain"

  controls.release = GainBias {
    button      = "R",
    branch      = branches.release,
    description = "Release",
    gainbias    = objects.release,
    range       = objects.releaseRange,
    biasMap     = Encoder.getMap("ADSR"),
    biasUnits   = app.unitSecs,
    initialBias = 0.100
  }
  views.expanded[#views.expanded + 1] = "release"
end

function MultiVoice:onLoadViews(objects, branches)
  local controls = {}
  local views = { expanded = {}, collapsed = {} }

  controls.scope = OutputScope {
    monitor = self,
    width = 4 * ply,
  }

  for i = 1, self.voiceCount do
    self:setVoiceViews(i, objects, branches, controls, views)
  end

  self:setPitchViews(objects, branches, controls, views)
  self:setFilterViews(objects, branches, controls, views)
  self:setAdsrViews(objects, branches, controls, views)

  return controls, views
end

return MultiVoice
