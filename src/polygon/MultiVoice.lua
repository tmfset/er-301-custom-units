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
  self.voiceCount = args.voiceCount or app.error("%s.init: voiceCount is missing.", self)
  self.oscType = args.oscType or app.error("%s.init: oscType is missing.", self)
  self.isSingleCycle = self.oscType == "SingleCycle"
  self.isPolyphonic = self.voiceCount > 1

  args.version = 2
  Unit.init(self, args)
end

function MultiVoice:createControls()
  local controls = {
    f0           = self:createObject("GainBias", "f0"),
    f0Range      = self:createObject("MinMax", "f0Range"),
    detune       = self:createObject("ConstantOffset", "detune"),
    detuneRange  = self:createObject("MinMax", "detuneRange"),
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
    releaseRange = self:createObject("MinMax", "releaseRange")
  }

  if self.isSingleCycle then
    controls.phase      = self:createObject("GainBias","phase")
    controls.phaseRange = self:createObject("MinMax","phaseRange")
    controls.scan       = self:createObject("GainBias","scan")
    controls.scanRange  = self:createObject("MinMax","scanRange")
  end

  if self.isPolyphonic then
    controls.rrCount     = self:createObject("Counter", "rrCount")
    controls.rrGate      = self:createObject("Comparator", "rrGate")
    controls.rrTune      = self:createObject("ConstantOffset", "rrTune")
    controls.rrTuneRange = self:createObject("MinMax", "rrTuneRange")
  end

  return controls
end

function MultiVoice:connectControls(controls)
  connect(controls.f0, "Out", controls.f0Range, "In")
  connect(controls.detune, "Out", controls.detuneRange, "In")
  connect(controls.cutoff, "Out", controls.cutoffRange, "In")
  connect(controls.res, "Out", controls.clipper, "In")
  connect(controls.clipper, "Out", controls.resRange, "In")
  connect(controls.fEnv, "Out", controls.fEnvRange, "In")
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
  self:createMonoBranch("fEnv", controls.fEnv, "In", controls.fEnv, "Out")
  self:createMonoBranch("attack", controls.attack, "In", controls.attack, "Out")
  self:createMonoBranch("decay", controls.decay, "In", controls.decay, "Out")
  self:createMonoBranch("sustain", controls.sustain, "In", controls.sustain, "Out")
  self:createMonoBranch("release", controls.release, "In", controls.release, "Out")

  if self.isSingleCycle then
    connect(controls.phase, "Out", controls.phaseRange, "In")
    connect(controls.scan, "Out", controls.scanRange, "In")

    self:createMonoBranch("phase", controls.phase, "In", controls.phase, "Out")
    self:createMonoBranch("scan", controls.scan, "In", controls.scan, "Out")
  end

  if self.isPolyphonic then
    local switch = self:createObject("Comparator", "switch")
    switch:hardSet("Threshold", -0.5)
    
    -- Invert the gate here since the Comparator triggerOnFallMode doesn't seem
    -- to be working.
    local invert = self:createObject("Multiply", "invert")
    local negOne = self:createObject("Constant", "negOne")
    negOne:hardSet("Value", -1.0)
    connect(controls.rrGate, "Out", invert, "Left")
    connect(negOne, "Out", invert, "Right")

    connect(invert, "Out", switch, "In")
    switch:setTriggerMode()

    connect(switch, "Out", controls.rrCount, "In")
    controls.rrCount:hardSet("Start", 0)
    controls.rrCount:hardSet("Step Size", 1)
    controls.rrCount:hardSet("Finish", self.voiceCount - 1)
    controls.rrCount:hardSet("Gain", 1 / (self.voiceCount - 1))

    connect(controls.rrTune, "Out", controls.rrTuneRange, "In")
    controls.rrGate:setGateMode()

    self:createMonoBranch("rrGate", controls.rrGate, "In", controls.rrGate, "Out")
    self:createMonoBranch("rrTune", controls.rrTune, "In", controls.rrTune, "Out")
  end
end

function MultiVoice:createVoice(i)
  local voice = {
    index      = i,

    -- Dedicated gate / tune
    dGate      = self:createObject("Comparator", "dGate"..i),
    dTune      = self:createObject("ConstantOffset", "dTune"..i),
    dTuneRange = self:createObject("MinMax", "dTuneRange"..i),

    oscA       = self:createObject(self.oscType, "oscA"..i),
    oscB       = self:createObject(self.oscType, "oscB"..i),
    detuneSum  = self:createObject("Sum", "detuneSum"..i),
    mixer      = self:createObject("Mixer", "mixer"..i, 2),
    adsr       = self:createObject("ADSR", "adsr"..i),
    vca        = self:createObject("Multiply", "vca"..i),
    fEnvVca    = self:createObject("Multiply", "fEnvVca"..i),
    filter     = self:createObject("StereoLadderFilter", "filter"..i)
  }

  if self.isPolyphonic then
    voice.rrIsActive = self:createObject("BumpMap", "rrIsActive"..i)
    voice.rrGate     = self:createObject("Multiply", "rrGate"..i)
    voice.cTune      = self:createObject("Sum", "cTune"..i)
    voice.gate       = self:createObject("Sum", "gate"..i)
    voice.tune       = self:createObject("TrackAndHold", "tune"..i)
  end

  return voice
end

function MultiVoice:connectVoice(voice, controls)
  -- Configure our dedicated gate / tune
  connect(voice.dTune, "Out", voice.dTuneRange, "In")
  voice.dGate:setGateMode()
  self:createMonoBranch("dGate"..voice.index, voice.dGate, "In", voice.dGate, "Out")
  self:createMonoBranch("dTune"..voice.index, voice.dTune, "In", voice.dTune, "Out")

  if self.isPolyphonic then
    -- The round robin count determines if we are active
    connect(controls.rrCount, "Out", voice.rrIsActive, "In")
    voice.rrIsActive:hardSet("Center", (voice.index - 1) / (self.voiceCount - 1))
    voice.rrIsActive:hardSet("Width", 1 / ((self.voiceCount - 1) * 2))
    voice.rrIsActive:hardSet("Height", 1)
    voice.rrIsActive:hardSet("Fade", 0.001)

    -- Bypass the round robin gate if not active
    connect(controls.rrGate, "Out", voice.rrGate, "Left")
    connect(voice.rrIsActive, "Out", voice.rrGate, "Right")

    -- Combine dedicated and round robin gate / tune
    connect(voice.dGate, "Out", voice.gate, "Left")
    connect(voice.rrGate, "Out", voice.gate, "Right")
    connect(voice.dTune, "Out", voice.cTune, "Left")
    connect(controls.rrTune, "Out", voice.cTune, "Right")

    -- Hold the actual tune after the combined gate drops
    connect(voice.cTune, "Out", voice.tune, "In")
    connect(voice.gate, "Out", voice.tune, "Track")

    -- Actual gate / tune goes out
    connect(voice.gate, "Out", voice.adsr, "Gate")
    connect(voice.tune, "Out", voice.detuneSum, "Left")
    connect(voice.tune, "Out", voice.oscA, "V/Oct")
  else
    -- Dedicated gate / tune goes out
    connect(voice.dGate, "Out", voice.adsr, "Gate")
    connect(voice.dTune, "Out", voice.detuneSum, "Left")
    connect(voice.dTune, "Out", voice.oscA, "V/Oct")
  end

  -- Oscillator settings
  connect(controls.f0, "Out", voice.oscA, "Fundamental")
  connect(controls.f0, "Out", voice.oscB, "Fundamental")
  connect(controls.detune, "Out", voice.detuneSum, "Right")
  connect(voice.detuneSum, "Out", voice.oscB, "V/Oct")

  if self.isSingleCycle then
    connect(controls.phase, "Out", voice.oscA, "Phase")
    connect(controls.phase, "Out", voice.oscB, "Phase")
    connect(controls.scan, "Out", voice.oscA, "Slice Select")
    connect(controls.scan, "Out", voice.oscB, "Slice Select")
  end

  -- Oscillator mix
  connect(voice.oscA, "Out", voice.mixer, "In1")
  connect(voice.oscB, "Out", voice.mixer, "In2")
  voice.mixer:hardSet("Gain1", 0.5)
  voice.mixer:hardSet("Gain2", 0.5)

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

function MultiVoice:setVoiceSample(sample, i)
  if sample == nil or sample:getChannelCount()==0 then
    self.objects["oscA"..i]:setSample(nil, nil)
    self.objects["oscB"..i]:setSample(nil, nil)
  else
    self.objects["oscA"..i]:setSample(sample.pSample,sample.slices.pSlices)
    self.objects["oscB"..i]:setSample(sample.pSample,sample.slices.pSlices)
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
      self.slicingView = SlicingView(self, self.objects["oscA1"])
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
  local chooser = SamplePoolInterface(self.loadInfo.id,"choose")
  chooser:setDefaultChannelCount(self.channelCount)
  chooser:highlight(self.sample)

  local task = function(sample)
    if sample then
      local Overlay = require "Overlay"
      Overlay.mainFlashMessage("Attached sample: %s",sample.name)
      self:setSample(sample)
    end
  end

  chooser:subscribe("done", task)
  chooser:show()
end

function MultiVoice:showVoiceInputs()
  self:switchView("voiceInputs")
end

function MultiVoice:showExpanded()
  self:switchView("expanded")
end

function MultiVoice:setExtendedMenu(objects, branches, controls, menu, sub)
  controls.extendedControlsHeader = MenuHeader {
    description = "Show View"
  }
  menu[#menu + 1] = "extendedControlsHeader"

  if self.isPolyphonic then
    controls.showNormalView = Task {
      description = "normal",
      task        = function() self:showExpanded() end
    }
    menu[#menu + 1] = "showNormalView"

    controls.showVoiceInputs = Task {
      description = "voice inputs",
      task        = function() self:showVoiceInputs() end
    }
    menu[#menu + 1] = "showVoiceInputs"
  end
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

  self:setExtendedMenu(objects, branches, controls, menu, sub)

  return controls, menu, sub
end

function MultiVoice:setRoundRobinViews(objects, branches, controls, views)
  if self.isPolyphonic then
    controls.rrGate = Gate {
      button      = "gate",
      description = "Round Robin Gate",
      branch      = branches.rrGate,
      comparator  = objects.rrGate
    }
    views.expanded[#views.expanded + 1] = "rrGate"

    controls.rrTune = Pitch {
      button      = "V/oct",
      description = "Round Robin V/oct",
      branch      = branches.rrTune,
      offset      = objects.rrTune,
      range       = objects.rrTuneRange
    }
    views.expanded[#views.expanded + 1] = "rrTune"
  end
end

function MultiVoice:setVoiceViews(objects, branches, controls, views)
  for i = 1, self.voiceCount do
    controls["gate"..i] = Gate {
      button      = "gate"..i,
      description = "Gate "..i,
      branch      = branches["dGate"..i],
      comparator  = objects["dGate"..i]
    }

    controls["tune"..i] = Pitch {
      button      = "V/oct"..i,
      description = "V/oct "..i,
      branch      = branches["dTune"..i],
      offset      = objects["dTune"..i],
      range       = objects["dTuneRange"..i]
    }

    if self.isPolyphonic then
      views.voiceInputs[#views.voiceInputs + 1] = "gate"..i
      views.voiceInputs[#views.voiceInputs + 1] = "tune"..i
    else
      views.expanded[#views.expanded + 1] = "gate"..i
      views.expanded[#views.expanded + 1] = "tune"..i
    end
  end
end

function MultiVoice:setOscViews(objects, branches, controls, views)
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

  if self.isSingleCycle then
    controls.phase = GainBias {
      button      = "phase",
      description = "Phase Offset",
      branch      = branches.phase,
      gainbias    = objects.phase,
      range       = objects.phaseRange,
      biasMap     = Encoder.getMap("[-1,1]"),
      initialBias = 0.0,
    }
    views.expanded[#views.expanded + 1] = "phase"

    controls.scan = GainBias {
      button      = "scan",
      description = "Table Scan",
      branch      = branches.scan,
      gainbias    = objects.scan,
      range       = objects.scanRange,
      biasMap     = Encoder.getMap("[0,1]"),
      initialBias = 0,
    }
    views.expanded[#views.expanded + 1] = "scan"
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
    initialBias = 0.25,
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
    initialBias = 0.5
  }
  views.expanded[#views.expanded + 1] = "fEnv"
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
  local views = { expanded = {}, voiceInputs = {}, collapsed = { "scope" } }

  controls.scope = OutputScope {
    monitor = self,
    width = 4 * ply,
  }

  self:setRoundRobinViews(objects, branches, controls, views)
  self:setVoiceViews(objects, branches, controls, views)
  self:setOscViews(objects, branches, controls, views)
  self:setFilterViews(objects, branches, controls, views)
  self:setAdsrViews(objects, branches, controls, views)

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
