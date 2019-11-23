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

  args.version = 1
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

  if self.isSingleCycle then
    controls.phase      = self:createObject("GainBias","phase")
    controls.phaseRange = self:createObject("MinMax","phaseRange")
    controls.scan       = self:createObject("GainBias","scan")
    controls.scanRange  = self:createObject("MinMax","scanRange")
  end

  return controls
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

  if self.isSingleCycle then
    connect(controls.phase, "Out", controls.phaseRange, "In")
    connect(controls.scan, "Out", controls.scanRange, "In")

    self:createMonoBranch("phase", controls.phase, "In", controls.phase, "Out")
    self:createMonoBranch("scan", controls.scan, "In", controls.scan, "Out")
  end
end

function MultiVoice:createVoice(i)
  return {
    index     = i,
    tune      = self:createObject("ConstantOffset", "tune"..i),
    tuneRange = self:createObject("MinMax", "tuneRange"..i),
    oscA      = self:createObject(self.oscType, "oscA"..i),
    oscB      = self:createObject(self.oscType, "oscB"..i),
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

  connect(controls.attack, "Out", voice.adsr, "Attack")
  connect(controls.decay, "Out", voice.adsr, "Decay")
  connect(controls.sustain, "Out", voice.adsr, "Sustain")
  connect(controls.release, "Out", voice.adsr, "Release")

  connect(voice.mixer, "Out", voice.filter, "Left In")
  connect(voice.adsr, "Out", voice.fenvVca, "Left")
  connect(controls.fenv, "Out", voice.fenvVca, "Right")
  connect(voice.fenvVca, "Out", voice.filter, "V/Oct")
  connect(controls.cutoff, "Out", voice.filter, "Fundamental")
  connect(controls.clipper, "Out", voice.filter, "Resonance")

  connect(voice.adsr, "Out", voice.vca, "Left")
  connect(voice.filter, "Left Out", voice.vca, "Right")

  self:createMonoBranch("gate"..voice.index, voice.gate, "In", voice.gate, "Out")
  self:createMonoBranch("tune"..voice.index, voice.tune, "In", voice.tune, "Out")

  if self.isSingleCycle then
    connect(controls.phase, "Out", voice.oscA, "Phase")
    connect(controls.phase, "Out", voice.oscB, "Phase")
    connect(controls.scan, "Out", voice.oscA, "Slice Select")
    connect(controls.scan, "Out", voice.oscB, "Slice Select")
  end

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
  if sample==nil or sample:getChannelCount()==0 then
    self.objects["oscA"..i]:setSample(nil, nil)
    self.objects["oscB"..i]:setSample(nil, nil)
  else
    self.objects["oscA"..i]:setSample(sample.pSample,sample.slices.pSlices)
    self.objects["oscB"..i]:setSample(sample.pSample,sample.slices.pSlices)
  end
end

function MultiVoice:setSample(sample)
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

function MultiVoice:onLoadMenu(objects, branches)
  if not self.isSingleCycle then
    return {}, {}, {}
  end

  local menu = {
    "sampleHeader",
    "selectFromCard",
    "selectFromPool",
    "detachBuffer",
    "editSample",
    "interpolation"
  }

  local controls = {
    sampleHeader = MenuHeader {
      description = "Sample Menu"
    },
    selectFromCard = Task {
      description = "Select from Card",
      task        = function() self:doAttachSampleFromCard() end
    },
    selectFromPool = Task {
      description = "Select from Pool",
      task        = function() self:doAttachSampleFromPool() end
    },
    detachBuffer = Task {
      description = "Detach Buffer",
      task        = function() self:doDetachSample() end
    },
    editSample = Task {
      description = "Edit Buffer",
      task        = function() self:showSampleEditor() end
    }
  }

  local sub = {}
  if self.sample then
    sub = {
      {
        position = app.GRID5_LINE1,
        justify  = app.justifyLeft,
        text     = "Attached Sample:"
      },
      {
        position = app.GRID5_LINE2,
        justify  = app.justifyLeft,
        text     = "+ "..self.sample:getFilenameForDisplay(24)
      },
      {
        position = app.GRID5_LINE3,
        justify  = app.justifyLeft,
        text     = "+ "..self.sample:getDurationText()
      },
      {
        position = app.GRID5_LINE4,
        justify  = app.justifyLeft,
        text     = string.format("+ %s %s %s", self.sample:getChannelText(), self.sample:getSampleRateText(), self.sample:getMemorySizeText())
      }
    }
  else
    sub = {
      {
        position = app.GRID5_LINE3,
        justify  = app.justifyCenter,
        text     = "No sample attached."
      }
    }
  end

  return controls, menu, sub
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

  self:setOscViews(objects, branches, controls, views)
  self:setFilterViews(objects, branches, controls, views)
  self:setAdsrViews(objects, branches, controls, views)

  return controls, views
end

return MultiVoice
