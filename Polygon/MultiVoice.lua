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
local SamplePool = require "Sample.Pool"
local SamplePoolInterface = require "Sample.Pool.Interface"
local SlicingView = require "SlicingView"
local Task = require "Unit.MenuControl.Task"
local MenuHeader = require "Unit.MenuControl.Header"
local ply = app.SECTION_PLY

local MultiVoice = Class{}
MultiVoice:include(Unit)

function MultiVoice:init(args)
  self.voiceCount    = args.voiceCount or app.error("%s.init: voiceCount is missing.", self)
  self.oscType       = args.oscType or app.error("%s.init: oscType is missing.", self)
  self.extraOscCount = args.extraOscCount or app.error("%s.init: extraOscCount is missing.", self)

  self.isSingleCycle     = self.oscType == "SingleCycle"
  self.isPolyphonic      = self.voiceCount > 1
  self.isMultiOscillator = self.extraOscCount > 0
  self.averageVoiceLevel = 1 / self.voiceCount
  self.averageOscLevel   = 1 / (1 + self.extraOscCount)

  args.version = 3
  Unit.init(self, args)
end

function MultiVoice:createControls()
  local controls = {
    hardSync     = self:createObject("Comparator", "hardSync"),
    f0           = self:createObject("GainBias", "f0"),
    f0Range      = self:createObject("MinMax", "f0Range"),
    cutoff       = self:createObject("GainBias", "cutoff"),
    cutoffRange  = self:createObject("MinMax", "cutoffRange"),
    res          = self:createObject("GainBias", "res"),
    resRange     = self:createObject("MinMax", "resRange"),
    clipper      = self:createObject("Clipper", "clipper"),
    fEnv         = self:createObject("GainBias", "fEnv"),
    fEnvRange    = self:createObject("MinMax", "fEnvRange"),
    attack       = self:createObject("GainBias", "attack"),
    attackRange  = self:createObject("MinMax", "attackRange"),
    decay        = self:createObject("GainBias", "decay"),
    decayRange   = self:createObject("MinMax", "decayRange"),
    sustain      = self:createObject("GainBias", "sustain"),
    sustainRange = self:createObject("MinMax", "sustainRange"),
    release      = self:createObject("GainBias", "release"),
    releaseRange = self:createObject("MinMax", "releaseRange"),
    level        = self:createObject("GainBias", "level"),
    levelRange   = self:createObject("MinMax", "levelRange"),
    levelVca     = self:createObject("Multiply", "levelVca"),

    negOne       = self:createObject("Constant", "negOne"),
    pointFive    = self:createObject("Constant", "pointFive")
  }

  for i = 1, self.extraOscCount do
    controls["sLevel"..i]      = self:createObject("ParameterAdapter", "sLevel"..i)
    controls["detune"..i]      = self:createObject("ConstantOffset", "detune"..i)
    controls["detuneRange"..i] = self:createObject("MinMax", "detuneRange"..i)

    if self.isSingleCycle then
      controls["phase"..i]      = self:createObject("GainBias", "phase"..i)
      controls["phaseRange"..i] = self:createObject("MinMax", "phaseRange"..i)
    else
      controls["phase"..i]      = self:createObject("ParameterAdapter", "phase"..i)
    end
  end

  if self.isSingleCycle then
    controls.scan      = self:createObject("GainBias", "scan")
    controls.scanRange = self:createObject("MinMax", "scanRange")
  end

  if self.isPolyphonic then
    controls.rrCount     = self:createObject("Counter", "rrCount")
    controls.rrGate      = self:createObject("Comparator", "rrGate")
    controls.rrSync      = self:createObject("Comparator", "rrSync")
    controls.rrTune      = self:createObject("ConstantOffset", "rrTune")
    controls.rrTuneRange = self:createObject("MinMax", "rrTuneRange")
  end

  return controls
end

function MultiVoice:connectControls(controls)
  controls.negOne:hardSet("Value", -1.0)
  controls.pointFive:hardSet("Value", 0.5)

  connect(controls.f0, "Out", controls.f0Range, "In")
  connect(controls.cutoff, "Out", controls.cutoffRange, "In")
  connect(controls.res, "Out", controls.clipper, "In")
  connect(controls.clipper, "Out", controls.resRange, "In")
  connect(controls.fEnv, "Out", controls.fEnvRange, "In")
  connect(controls.attack, "Out", controls.attackRange, "In")
  connect(controls.decay, "Out", controls.decayRange, "In")
  connect(controls.sustain, "Out", controls.sustainRange, "In")
  connect(controls.release, "Out", controls.releaseRange, "In")
  connect(controls.level, "Out", controls.levelRange, "In")

  controls.hardSync:setToggleMode()
  controls.clipper:setMaximum(0.999)
  controls.clipper:setMinimum(0)

  self:createMonoBranch("hardSync", controls.hardSync, "In", controls.hardSync, "Out")
  self:createMonoBranch("f0", controls.f0, "In", controls.f0, "Out")
  self:createMonoBranch("cutoff", controls.cutoff, "In", controls.cutoff, "Out")
  self:createMonoBranch("Q", controls.res, "In", controls.res, "Out")
  self:createMonoBranch("fEnv", controls.fEnv, "In", controls.fEnv, "Out")
  self:createMonoBranch("attack", controls.attack, "In", controls.attack, "Out")
  self:createMonoBranch("decay", controls.decay, "In", controls.decay, "Out")
  self:createMonoBranch("sustain", controls.sustain, "In", controls.sustain, "Out")
  self:createMonoBranch("release", controls.release, "In", controls.release, "Out")
  self:createMonoBranch("level", controls.level, "In", controls.level, "Out")

  for i = 1, self.extraOscCount do
    connect(controls["detune"..i], "Out", controls["detuneRange"..i], "In")

    self:createMonoBranch("sLevel"..i, controls["sLevel"..i], "In", controls["sLevel"..i], "Out")
    self:createMonoBranch("detune"..i, controls["detune"..i], "In", controls["detune"..i], "Out")

    if self.isSingleCycle then
      connect(controls["phase"..i], "Out", controls["phaseRange"..i], "In")
    end
    self:createMonoBranch("phase"..i, controls["phase"..i], "In", controls["phase"..i], "Out")
  end

  if self.isSingleCycle then
    connect(controls.scan, "Out", controls.scanRange, "In")
    self:createMonoBranch("scan", controls.scan, "In", controls.scan, "Out")
  end

  if self.isPolyphonic then
    local switch = self:createObject("Comparator", "switch")
    switch:hardSet("Threshold", -0.5)
    
    -- Invert the gate here since the Comparator triggerOnFallMode doesn't seem
    -- to be working.
    local invert = self:createObject("Multiply", "invert")
    connect(controls.rrGate, "Out", invert, "Left")
    connect(controls.negOne, "Out", invert, "Right")

    connect(invert, "Out", switch, "In")
    switch:setTriggerMode()

    connect(switch, "Out", controls.rrCount, "In")
    controls.rrCount:hardSet("Start", 0)
    controls.rrCount:hardSet("Step Size", 1)
    controls.rrCount:hardSet("Finish", self.voiceCount - 1)
    controls.rrCount:hardSet("Gain", 1 / (self.voiceCount - 1))

    connect(controls.rrTune, "Out", controls.rrTuneRange, "In")
    controls.rrGate:setGateMode()
    controls.rrSync:setTriggerMode()

    self:createMonoBranch("rrGate", controls.rrGate, "In", controls.rrGate, "Out")
    self:createMonoBranch("rrSync", controls.rrSync, "In", controls.rrSync, "Out")
    self:createMonoBranch("rrTune", controls.rrTune, "In", controls.rrTune, "Out")
  end
end

function MultiVoice:createVoice(i)
  local voice = {
    index      = i,

    -- Dedicated gate / sync / tune
    dGate      = self:createObject("Comparator", "dGate"..i),
    dSync      = self:createObject("Comparator", "dSync"..i),
    dTune      = self:createObject("ConstantOffset", "dTune"..i),
    dTuneRange = self:createObject("MinMax", "dTuneRange"..i),

    -- Actual gate / sync / tune
    gate       = self:createObject("Sum", "gate"..i),
    sync       = self:createObject("Sum", "sync"..i),
    tune       = self:createObject("TrackAndHold", "tune"..i),

    -- Hard sync control
    syncVca    = self:createObject("Multiply", "syncVca"..i),
    syncComp   = self:createObject("Comparator", "syncComp"..i),
    hSyncSum   = self:createObject("Sum", "hSyncSum"..i),
    cSync      = self:createObject("Sum", "cSync"..i),

    -- Primary oscillator
    pOsc       = self:createObject(self.oscType, "pOsc"..i),
    mixer      = self:createObject("Mixer", "mixer"..i, 1 + self.extraOscCount),
    adsr       = self:createObject("ADSR", "adsr"..i),
    vca        = self:createObject("Multiply", "vca"..i),
    fEnvVca    = self:createObject("Multiply", "fEnvVca"..i),
    filter     = self:createObject("StereoLadderFilter", "filter"..i)
  }

  -- Secondary oscillators
  for j = 1, self.extraOscCount do
    voice["sOsc"..i..j]         = self:createObject(self.oscType, "sOsc"..i..j)
    voice["sOscSum"..i..j]      = self:createObject("Sum", "sOscSum"..i..j)
  end

  if self.isSingleCycle then
    voice.scan      = self:createObject("GainBias", "scan"..i)
    voice.scanRange = self:createObject("MinMax", "scanRange"..i)
    voice.scanSum   = self:createObject("Sum", "scanSum"..i)
  end

  if self.isPolyphonic then
    voice.rrIsActive = self:createObject("BumpMap", "rrIsActive"..i)
    voice.rrGate     = self:createObject("Multiply", "rrGate"..i)
    voice.rrSync     = self:createObject("Multiply", "rrSync"..i)
    voice.cTune      = self:createObject("Sum", "cTune"..i)
    voice.level      = self:createObject("ParameterAdapter", "level"..i)
  end

  return voice
end

function MultiVoice:connectVoice(voice, controls)
  -- Configure our dedicated gate / tune
  connect(voice.dTune, "Out", voice.dTuneRange, "In")
  voice.dGate:setGateMode()
  voice.dSync:setTriggerMode()
  self:createMonoBranch("dGate"..voice.index, voice.dGate, "In", voice.dGate, "Out")
  self:createMonoBranch("dSync"..voice.index, voice.dSync, "In", voice.dSync, "Out")
  self:createMonoBranch("dTune"..voice.index, voice.dTune, "In", voice.dTune, "Out")

  if self.isPolyphonic then
    -- The round robin count determines if we are active
    connect(controls.rrCount, "Out", voice.rrIsActive, "In")
    voice.rrIsActive:hardSet("Center", (voice.index - 1) / (self.voiceCount - 1))
    voice.rrIsActive:hardSet("Width", 1 / ((self.voiceCount - 1) * 2))
    voice.rrIsActive:hardSet("Height", 1)
    voice.rrIsActive:hardSet("Fade", 0.001)

    -- Bypass the round robin gate / sync if not active
    connect(controls.rrGate, "Out", voice.rrGate, "Left")
    connect(voice.rrIsActive, "Out", voice.rrGate, "Right")

    connect(controls.rrSync, "Out", voice.rrSync, "Left")
    connect(voice.rrIsActive, "Out", voice.rrSync, "Right")

    -- Combine dedicated and round robin gate / sync / tune
    connect(voice.dGate, "Out", voice.gate, "Left")
    connect(voice.rrGate, "Out", voice.gate, "Right")

    connect(voice.dSync, "Out", voice.sync, "Left")
    connect(voice.rrSync, "Out", voice.sync, "Right")

    connect(voice.dTune, "Out", voice.cTune, "Left")
    connect(controls.rrTune, "Out", voice.cTune, "Right")
    connect(voice.cTune, "Out", voice.tune, "In")
  else
    -- Dedicated gate / sync / tune goes out
    connect(voice.dGate, "Out", voice.gate, "Left")
    connect(voice.dSync, "Out", voice.sync, "Left")
    connect(voice.dTune, "Out", voice.tune, "In")
  end

  -- Hold the actual tune after the combined gate drops
  connect(voice.gate, "Out", voice.tune, "Track")
  connect(voice.gate, "Out", voice.adsr, "Gate")
  connect(voice.sync, "Out", voice.pOsc, "Sync")
  connect(voice.tune, "Out", voice.pOsc, "V/Oct")

  -- Oscillator settings
  connect(controls.f0, "Out", voice.pOsc, "Fundamental")
  connect(voice.pOsc, "Out", voice.mixer, "In1")

  -- Hard sync
  --connect(voice.pOsc, "Out", voice.sync, "In")
  -- voice.syncComp:setTriggerMode()
  -- connect(voice.pOsc, "Out", voice.hSyncSum, "Left")
  -- connect(controls.pointFive, "Out", voice.hSyncSum, "Right")
  -- connect(voice.hSyncSum, "Out", voice.syncComp, "In")
  -- connect(voice.syncComp, "Out", voice.syncVca, "Left")
  -- connect(controls.hardSync, "Out", voice.syncVca, "Right")
  -- connect(voice.sync, "Out", voice.cSync, "Left")
  -- connect(voice.syncVca, "Out", voice.cSync, "Right")

  -- Oscillator mix
  voice.mixer:hardSet("Gain1", self.averageOscLevel)

  for i = 1, self.extraOscCount do
    local sOsc = voice["sOsc"..voice.index..i]
    local sOscSum = voice["sOscSum"..voice.index..i]

    connect(controls.f0, "Out", sOsc, "Fundamental")
    connect(voice.tune, "Out", sOscSum, "Left")
    connect(controls["detune"..i], "Out", sOscSum, "Right")
    connect(sOscSum, "Out", sOsc, "V/Oct")
    connect(voice.cSync, "Out", sOsc, "Sync")

    connect(sOsc, "Out", voice.mixer, "In"..(1 + i))
    tie(voice.mixer, "Gain"..(1 + i), controls["sLevel"..i], "Out")

    if self.isSingleCycle then
      connect(controls["phase"..i], "Out", sOsc, "Phase")
      connect(voice.scanSum, "Out", sOsc, "Slice Select")
    end
  end

  if self.isSingleCycle then
    connect(controls.scan, "Out", voice.scanSum, "Left")
    connect(voice.scan, "Out", voice.scanSum, "Right")
    connect(voice.scanSum, "Out", voice.pOsc, "Slice Select")

    connect(voice.scan, "Out", voice.scanRange, "In")
    self:createMonoBranch("scan"..voice.index, voice.scan, "In", voice.scan, "Out")
  end

  -- ADSR
  connect(voice.adsr, "Out", voice.vca, "Left")
  connect(voice.adsr, "Out", voice.fEnvVca, "Left")
  connect(controls.attack, "Out", voice.adsr, "Attack")
  connect(controls.decay, "Out", voice.adsr, "Decay")
  connect(controls.sustain, "Out", voice.adsr, "Sustain")
  connect(controls.release, "Out", voice.adsr, "Release")

  -- Filter
  connect(controls.fEnv, "Out", voice.fEnvVca, "Right")
  connect(voice.fEnvVca, "Out", voice.filter, "V/Oct")
  connect(controls.cutoff, "Out", voice.filter, "Fundamental")
  connect(controls.clipper, "Out", voice.filter, "Resonance")

  -- Mix -> Filter -> VCA -> Out
  connect(voice.mixer, "Out", voice.filter, "Left In")
  connect(voice.filter, "Left Out", voice.vca, "Right")
  return voice.vca, "Out"
end

function MultiVoice:onLoadGraph(channelCount)
  local voiceMixer = self:createObject("Mixer", "voiceMixer", self.voiceCount)

  local controls = self:createControls()
  self:connectControls(controls)

  connect(controls.level, "Out", controls.levelVca, "Left")
  connect(voiceMixer, "Out", controls.levelVca, "Right")
  connect(controls.levelVca, "Out", self, "Out1")

  for i = 1, self.voiceCount do
    local voice = self:createVoice(i)
    local out, key = self:connectVoice(voice, controls)

    connect(out, key, voiceMixer, "In"..i)
    if self.isPolyphonic then
      tie(voiceMixer, "Gain"..i, voice.level, "Out")
      self:createMonoBranch("level"..i, voice.level, "In", voice.level, "Out")
    else
      voiceMixer:hardSet("Gain"..i, self.averageVoiceLevel)
    end
  end
end

function MultiVoice:setVoiceSample(sample, i)
  local pSample = nil
  local pSlices = nil

  local isEmpty = sample == nil or sample:getChannelCount() == 0
  if not isEmpty then
    pSample = sample.pSample
    pSlices = sample.slices.pSlices
  end

  self.objects["pOsc"..i]:setSample(pSample, pSlices)
  for j = 1, self.extraOscCount do
    self.objects["sOsc"..i..j]:setSample(pSample, pSlices)
  end
end

function MultiVoice:setSample(sample)
  if not self.isSingleCycle then
    return
  end

  if self.sample then
    self.sample:release(self)
    self.sample = nil
  end

  self.sample = sample
  if self.sample then
    self.sample:claim(self)
  end

  for i = 1, self.voiceCount do
    self:setVoiceSample(self.sample, i)
  end

  if self.slicingView then
    self.slicingView:setSample(sample)
  end

  self:notifyControls("setSample",sample)
end

function MultiVoice:showSampleEditor()
  if self.sample then
    if self.slicingView == nil then
      self.slicingView = SlicingView(self, self.objects["pOsc1"])
      self.slicingView:setSample(self.sample)
    end
    self.slicingView:show()
  else
    local Overlay = require "Overlay"
    Overlay.mainFlashMessage("You must first select a sample.")
  end
end

function MultiVoice:doDetachSample()
  local Overlay = require "Overlay"
  Overlay.mainFlashMessage("Sample detached.")
  self:setSample()
end

function MultiVoice:doAttachSampleFromCard()
  local task = function(sample)
    if sample then
      local Overlay = require "Overlay"
      Overlay.mainFlashMessage("Attached sample: %s",sample.name)
      self:setSample(sample)
    end
  end

  local Pool = require "Sample.Pool"
  Pool.chooseFileFromCard(self.loadInfo.id,task)
end

function MultiVoice:doAttachSampleFromPool()
  local chooser = SamplePoolInterface(self.loadInfo.id, "choose")
  chooser:setDefaultChannelCount(self.channelCount)
  chooser:highlight(self.sample)

  local task = function(sample)
    if sample then
      local Overlay = require "Overlay"
      Overlay.mainFlashMessage("Attached sample: %s", sample.name)
      self:setSample(sample)
    end
  end

  chooser:subscribe("done", task)
  chooser:show()
end

function MultiVoice:setSampleMenu(objects, branches, controls, menu, sub)
  controls.sampleHeader = MenuHeader {
    description = "Sample Menu"
  }
  menu[#menu + 1] = "sampleHeader"

  controls.selectFromCard = Task {
    description = "Select from Card",
    task        = function() self:doAttachSampleFromCard() end
  }
  menu[#menu + 1] = "selectFromCard"

  controls.selectFromPool = Task {
    description = "Select from Pool",
    task        = function() self:doAttachSampleFromPool() end
  }
  menu[#menu + 1] = "selectFromPool"

  controls.detachBuffer = Task {
    description = "Detach Buffer",
    task        = function() self:doDetachSample() end
  }
  menu[#menu + 1] = "detachBuffer"

  controls.editSample = Task {
    description = "Edit Buffer",
    task        = function() self:showSampleEditor() end
  }
  menu[#menu + 1] = "editSample"

  if self.sample then
    sub[#sub + 1] = {
      position = app.GRID5_LINE1,
      justify  = app.justifyLeft,
      text     = "Attached Sample:"
    }

    sub[#sub + 1] = {
      position = app.GRID5_LINE2,
      justify  = app.justifyLeft,
      text     = "+ "..self.sample:getFilenameForDisplay(24)
    }

    sub[#sub + 1] = {
      position = app.GRID5_LINE3,
      justify  = app.justifyLeft,
      text     = "+ "..self.sample:getDurationText()
    }

    sub[#sub + 1] = {
      position = app.GRID5_LINE4,
      justify  = app.justifyLeft,
      text     = string.format("+ %s %s %s", self.sample:getChannelText(), self.sample:getSampleRateText(), self.sample:getMemorySizeText())
    }
  else
    sub[#sub + 1] = {
      position = app.GRID5_LINE3,
      justify  = app.justifyCenter,
      text     = "No sample attached."
    }
  end
end

function MultiVoice:onLoadMenu(objects, branches)
  local controls, menu, sub = {}, {}, {}

  if self.isSingleCycle then
    self:setSampleMenu(objects, branches, controls, menu, sub)
  end

  return controls, menu, sub
end

function MultiVoice:setGateViews(objects, branches, controls, views)
  if self.isPolyphonic then
    controls.gateR = Gate {
      button      = "gateR",
      description = "Round Robin Gate",
      branch      = branches.rrGate,
      comparator  = objects.rrGate
    }
    views.expanded[#views.expanded + 1] = "gateR"
    views.gateR = { "gateR" }

    for i = 1, self.voiceCount do
      controls["gate"..i] = Gate {
        button      = "gate"..i,
        description = "Voice "..i.." Gate",
        branch      = branches["dGate"..i],
        comparator  = objects["dGate"..i]
      }
      views.gateR[#views.gateR + 1] = "gate"..i
      views["gate"..i] = views.expanded
    end
  else
    controls.gate = Gate {
      button      = "gate",
      description = "Gate",
      branch      = branches["dGate"..1],
      comparator  = objects["dGate"..1]
    }
    views.expanded[#views.expanded + 1] = "gate"
  end
end

function MultiVoice:setTuneViews(objects, branches, controls, views)
  if self.isPolyphonic then
    controls.tuneR = Pitch {
      button      = "V/octR",
      description = "Round Robin V/oct",
      branch      = branches.rrTune,
      offset      = objects.rrTune,
      range       = objects.rrTuneRange
    }
    views.expanded[#views.expanded + 1] = "tuneR"
    views.tuneR = { "tuneR" }

    for i = 1, self.voiceCount do
      controls["tune"..i] = Pitch {
        button      = "V/oct"..i,
        description = "Voice "..i.." V/oct ",
        branch      = branches["dTune"..i],
        offset      = objects["dTune"..i],
        range       = objects["dTuneRange"..i]
      }
      views.tuneR[#views.tuneR + 1] = "tune"..i
      views["tune"..i] = views.expanded
    end
  else
    controls.tune = Pitch {
      button      = "V/octR",
      description = "V/oct",
      branch      = branches["dTune"..1],
      offset      = objects["dTune"..1],
      range       = objects["dTuneRange"..1]
    }
    views.expanded[#views.expanded + 1] = "tune"
  end
end

function MultiVoice:setSyncViews(objects, branches, controls, views)
  if self.isPolyphonic then
    controls.syncR = Gate {
      button      = "syncR",
      description = "Round Robin Sync",
      branch      = branches.rrSync,
      comparator  = objects.rrSync
    }
    views.expanded[#views.expanded + 1] = "syncR"
    views.syncR = { "syncR" }

    controls.hSync = Gate {
      button      = "hSync",
      description = "Hard Sync",
      branch      = branches.hardSync,
      comparator  = objects.hardSync
    }
    views.hSync = views.expanded
    views.syncR[#views.syncR + 1] = "hSync"

    for i = 1, self.voiceCount do
      controls["sync"..i] = Gate {
        button      = "sync"..i,
        description = "Voice "..i.." Sync",
        branch      = branches["dSync"..i],
        comparator  = objects["dSync"..i]
      }
      views.syncR[#views.syncR + 1] = "sync"..i
    end
  else
    controls.sync = Gate {
      button      = "sync",
      description = "Sync",
      branch      = branches["dSync"..1],
      comparator  = objects["dSync"..1]
    }
    views.expanded[#views.expanded + 1] = "sync"

    controls.hSync = Gate {
      button      = "hSync",
      description = "Hard Sync",
      branch      = branches.hardSync,
      comparator  = objects.hardSync
    }
    views.expanded[#views.expanded + 1] = "hSync"
  end
end

function MultiVoice:setFreqViews(objects, branches, controls, views)
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
  views.freq = { "freq", "scope2" }

  for i = 1, self.extraOscCount do
    controls["detune"..i] = Pitch {
      button      = "detune"..i,
      description = "Oscillator "..i.." Detune",
      branch      = branches["detune"..i],
      offset      = objects["detune"..i],
      range       = objects["detuneRange"..i]
    }
    views.freq[#views.freq + 1] = "detune"..i
    views["detune"..i] = views.expanded

    if self.isSingleCycle then
      controls["phase"..i] = GainBias {
        button      = "phase"..i,
        description = "Oscillator "..i.." Phase",
        branch      = branches["phase"..i],
        gainbias    = objects["phase"..i],
        range       = objects["phaseRange"..i],
        biasMap     = Encoder.getMap("[-1,1]"),
        initialBias = 0.0,
      }
      views.freq[#views.freq + 1] = "phase"..i
      views["phase"..i] = views.expanded
    end

    controls["oLvl"..i] = GainBias {
      button      = "oLvl"..i,
      description = "Oscillator "..i.." Level",
      branch      = branches["sLevel"..i],
      gainbias    = objects["sLevel"..i],
      range       = objects["sLevel"..i],
      biasMap     = Encoder.getMap("[0,1]"),
      initialBias = self.averageOscLevel
    }
    views.freq[#views.freq + 1] = "oLvl"..i
    views["oLvl"..i] = views.expanded
  end
end

function MultiVoice:setScanViews(objects, branches, controls, views)
  if not self.isSingleCycle then
    return
  end

  controls.scan = GainBias {
    button      = "scan",
    description = "Base Sample Scan",
    branch      = branches.scan,
    gainbias    = objects.scan,
    range       = objects.scanRange,
    biasMap     = Encoder.getMap("[0,1]"),
    initialBias = 0
  }
  views.expanded[#views.expanded + 1] = "scan"
  views.scan = { "scan" }

  for i = 1, self.voiceCount do
    controls["scan"..i] = GainBias {
      button      = "scan"..i,
      description = "Sample Scan Offset",
      branch      = branches["scan"..i],
      gainbias    = objects["scan"..i],
      range       = objects["scanRange"..i],
      biasMap     = Encoder.getMap("[-1,1]"),
      initialBias = 0
    }
    views.scan[#views.scan + 1] = "scan"..i
    views["scan"..i] = views.expanded
  end
end

function MultiVoice:setLevelViews(objects, branches, controls, views)
  controls.level = GainBias {
    button      = "level",
    description = "Output Level",
    branch      = branches.level,
    gainbias    = objects.level,
    range       = objects.levelRange,
    biasMap     = Encoder.getMap("[0,2]"),
    initialBias = 1.0
  }
  views.expanded[#views.expanded + 1] = "level"
  views.level = { "scope", "level" }

  if not self.isPolyphonic then
    return
  end

  for i = 1, self.voiceCount do
    controls["level"..i] = GainBias {
      button      = "vLvl"..i,
      description = "Voice "..i.." Level",
      branch      = branches["level"..i],
      gainbias    = objects["level"..i],
      range       = objects["level"..i],
      biasMap     = Encoder.getMap("[0,1]"),
      initialBias = self.averageVoiceLevel
    }
    views.level[#views.level + 1] = "level"..i
    views["level"..i] = views.expanded
  end
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
    initialBias = 0,
    gainMap     = Encoder.getMap("[-10,10]")
  }
  views.expanded[#views.expanded + 1] = "resonance"

  controls.fEnv = GainBias {
    button      = "fenv",
    description = "Filter Env Amount",
    branch      = branches.fEnv,
    gainbias    = objects.fEnv,
    range       = objects.fEnvRange,
    biasMap     = Encoder.getMap("[-1,1]"),
    initialBias = 0.3
  }
  views.expanded[#views.expanded + 1] = "fEnv"

  views.cutoff    = { "scope", "cutoff", "resonance", "fEnv" }
  views.resonance = { "scope", "cutoff", "resonance", "fEnv" }
  views.fEnv      = { "scope", "cutoff", "resonance", "fEnv" }
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
    initialBias = 1.000
  }
  views.expanded[#views.expanded + 1] = "release"

  views.attack  = { "scope", "attack" }
  views.decay   = { "scope", "decay" }
  views.sustain = { "scope", "sustain" }
  views.release = { "scope", "release" }
end

function MultiVoice:onLoadViews(objects, branches)
  local controls = {}
  local views = { expanded = {}, voiceInputs = {}, collapsed = { "scope" } }

  controls.scope = OutputScope {
    monitor = self,
    width = 4 * ply,
  }

  controls.scope2 = OutputScope {
    monitor = self,
    width = 2 * ply,
  }

  self:setGateViews(objects, branches, controls, views)
  self:setSyncViews(objects, branches, controls, views)
  self:setTuneViews(objects, branches, controls, views)
  self:setFreqViews(objects, branches, controls, views)
  self:setScanViews(objects, branches, controls, views)
  self:setFilterViews(objects, branches, controls, views)
  self:setAdsrViews(objects, branches, controls, views)
  self:setLevelViews(objects, branches, controls, views)

  return controls, views
end

function MultiVoice:serialize()
  local t = Unit.serialize(self)
  local sample = self.sample
  if sample then
    t.sample = SamplePool.serializeSample(sample)
  end
  return t
end

function MultiVoice:deserialize(t)
  Unit.deserialize(self,t)
  if t.sample then
    local sample = SamplePool.deserializeSample(t.sample)
    if sample then
      self:setSample(sample)
    else
      local Utils = require "Utils"
      app.log("%s:deserialize: failed to load sample.",self)
      Utils.pp(t.sample)
    end
  end
end

function MultiVoice:onRemove()
  self:setSample(nil)
  Unit.onRemove(self)
end

return MultiVoice
