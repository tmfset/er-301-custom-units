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

local MultiOsc = Class {}
MultiOsc:include(Unit)

function MultiOsc:init(args)
  self.oscType  = args.oscType or app.error("%s.init: oscType is missing.", self)
  self.oscCount = args.oscCount or app.error("%s.init: oscCount is missing.", self)

  self.isSingleCycle   = self.oscType == "SingleCycle"
  self.averageOscLevel = 1 / self.oscCount

  args.version = 1
  Unit.init(self, args)
end

function MultiOsc:createControls()
  local objects = {
    f0         = self:createObject("GainBias", "f0"),
    f0Range    = self:createObject("MinMax", "f0Range"),
    level      = self:createObject("GainBias", "level"),
    levelRange = self:createObject("MinMax", "levelRange"),
    sync       = self:createObject("Comparator", "sync"),
    hardSync   = self:createObject("Comparator", "hardSync")
  }

  connect(objects.f0, "Out", objects.f0Range, "In")
  connect(objects.level, "Out", objects.levelRange, "In")
  self:createMonoBranch("f0", objects.f0, "In", objects.f0, "Out")
  self:createMonoBranch("level", objects.level, "In", objects.level, "Out")
  self:createMonoBranch("sync", objects.sync, "In", objects.sync, "Out")
  self:createMonoBranch("hardSync", objects.hardSync, "In", objects.hardSync, "Out")

  objects.sync:setTriggerMode()
  objects.hardSync:setToggleMode()

  return objects
end

function MultiOsc:createOsc(type, suffix)
  local objects = {
    osc        = self:createObject(type, "osc"..suffix),
    tune       = self:createObject("ConstantOffset", "tune"..suffix),
    tuneRange  = self:createObject("MinMax", "tuneRange"..suffix),
    level      = self:createObject("GainBias", "level"..suffix),
    levelRange = self:createObject("MinMax", "levelRange"..suffix)
  }

  connect(objects.tune, "Out", objects.tuneRange, "In")
  connect(objects.level, "Out", objects.levelRange, "In")
  self:createMonoBranch("tune"..suffix, objects.tune, "In", objects.tune, "Out")
  self:createMonoBranch("level"..suffix, objects.level, "In", objects.level, "Out")

  if self.isSingleCycle then
    objects.scan       = self:createObject("GainBias", "scan"..suffix)
    objects.scanRange  = self:createObject("MinMax", "scanRange"..suffix)
    objects.phase      = self:createObject("GainBias", "phase"..suffix)
    objects.phaseRange = self:createObject("MinMax", "phaseRange"..suffix)

    connect(objects.scan, "Out", objects.scanRange, "In")
    connect(objects.phase, "Out", objects.phaseRange, "In")
    self:createMonoBranch("scan"..suffix, objects.scan, "In", objects.scan, "Out")
    self:createMonoBranch("phase"..suffix, objects.phase, "In", objects.phase, "Out")
  end

  return objects
end

function MultiOsc:createHardSync(args)
  local clock = self:createObject("ClockInHertz","clock")
  tie(clock, "Frequency", freq, "Out")
  local pointFive = self:createObject("Constant", "pointFive")
  pointFive:hardSet("Value", 0.5)

  local offset = self:createObject("Sum", "hSyncOffset")
  connect(args.osc, "Out", offset, "Left")
  connect(pointFive, "Out", offset, "Right")

  local detect = self:createObject("Comparator", "hSyncDetect")
  detect:setTriggerMode()
  connect(offset, "Out", detect, "In")

  local bypass = self:createObject("Multiply", "hSyncBypass")
  connect(args.enabled, "Out", bypass, "Left")
  connect(detect, "Out", bypass, "Right")
  
  local combined = self:createObject("Sum", "hSyncCombined")
  connect(args.sync, "Out", combined, "Left")
  connect(bypass, "Out", combined, "Right")

  return combined
end

function MultiOsc:onLoadGraph(channelCount)
  local controls = self:createControls()

  local oscCount = self:createObject("Constant", "oscCount")
  oscCount:hardSet("Value", 4)

  local levelSum = self:createObject("Mixer", "levelSum", self.oscCount)
  local mixer = self:createObject("Mixer", "mixer", self.oscCount)
  local agc = self:createObject("RationalMultiply", "agc")
  connect(mixer, "Out", agc, "In")
  connect(oscCount, "Out", agc, "Numerator")
  connect(levelSum, "Out", agc, "Divisor")

  local levelVca = self:createObject("Multiply", "levelVca")
  connect(controls.level, "Out", levelVca, "Left")
  connect(agc, "Out", levelVca, "Right")
  connect(levelVca, "Out", self, "Out1")

  local oscillators = {}
  for i = 1, self.oscCount do
    oscillators[i] = self:createOsc(self.oscType, i)
  end

  local primary = oscillators[1]

  local hardSync = self:createHardSync({
    osc     = primary.osc,
    sync    = controls.sync,
    enabled = controls.hardSync
  })

  for i = 1, self.oscCount do
    local current = oscillators[i]

    connect(controls.f0, "Out", current.osc, "Fundamental")

    local tune, sync
    if i == 1 then
      tune = primary.tune
      sync = controls.sync
    else
      tune = self:createObject("Sum", "tuneSum"..i)
      connect(primary.tune, "Out", tune, "Left")
      connect(current.tune, "Out", tune, "Right")

      sync = hardSync
    end

    connect(sync, "Out", current.osc, "Sync")
    connect(tune, "Out", current.osc, "V/Oct")

    if self.isSingleCycle then
      local scan, phase
      if i == 1 then
        scan  = primary.scan
        phase = primary.phase
      else
        scan = self:createObject("Sum", "scanSum"..i)
        connect(primary.scan, "Out", scan, "Left")
        connect(current.scan, "Out", scan, "Right")

        phase = self:createObject("Sum", "phaseSum"..i)
        connect(primary.phase, "Out", phase, "Left")
        connect(current.phase, "Out", phase, "Right")
      end

      connect(scan, "Out", current.osc, "Slice Select")
      connect(phase, "Out", current.osc, "Phase")
    end

    local vca = self:createObject("Multiply", "vca"..i)
    connect(current.level, "Out", vca, "Left")
    connect(current.osc, "Out", vca, "Right")

    connect(current.level, "Out", levelSum, "In"..i)
    levelSum:hardSet("Gain"..i, 1)

    connect(vca, "Out", mixer, "In"..i)
    mixer:hardSet("Gain"..i, self.averageOscLevel)
  end
end

function MultiOsc:setSample(sample)
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

  local pSample = nil
  local pSlices = nil

  local isEmpty = sample == nil or sample:getChannelCount() == 0
  if not isEmpty then
    pSample = sample.pSample
    pSlices = sample.slices.pSlices
  end

  for i = 1, self.oscCount do
    self.objects["osc"..i]:setSample(pSample, pSlices)
  end

  if self.slicingView then
    self.slicingView:setSample(sample)
  end

  self:notifyControls("setSample",sample)
end

function MultiOsc:showSampleEditor()
  if self.sample then
    if self.slicingView == nil then
      self.slicingView = SlicingView(self, self.objects["osc1"])
      self.slicingView:setSample(self.sample)
    end
    self.slicingView:show()
  else
    local Overlay = require "Overlay"
    Overlay.mainFlashMessage("You must first select a sample.")
  end
end

function MultiOsc:doDetachSample()
  local Overlay = require "Overlay"
  Overlay.mainFlashMessage("Sample detached.")
  self:setSample()
end

function MultiOsc:doAttachSampleFromCard()
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

function MultiOsc:doAttachSampleFromPool()
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

function MultiOsc:setSampleMenu(objects, branches, controls, menu, sub)
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

function MultiOsc:onLoadMenu(objects, branches)
  local controls, menu, sub = {}, {}, {}

  if self.isSingleCycle then
    self:setSampleMenu(objects, branches, controls, menu, sub)
  end

  return controls, menu, sub
end

function MultiOsc:tuneControl(objects, branches)
  return function (name, description, index)
    return Pitch {
      button      = name,
      description = description,
      branch      = branches["tune"..index],
      offset      = objects["tune"..index],
      range       = objects["tuneRange"..index]
    }
  end
end

function MultiOsc:gainControl(objects, branches)
  return function (name, description, index, bias, root, map)
    return GainBias {
      button      = name,
      description = description,
      branch      = branches[root..index],
      gainbias    = objects[root..index],
      range       = objects[root.."Range"..index],
      biasMap     = Encoder.getMap(map),
      initialBias = bias
    }
  end
end

function MultiOsc:levelControl(objects, branches)
  return function (name, description, index, bias)
    return self:gainControl(objects, branches)(name, description, index, bias, "level", "[0,1]")
  end
end

function MultiOsc:scanControl(objects, branches)
  return function (name, description, index)
    return self:gainControl(objects, branches)(name, description, index, 0, "scan", "[-1,1]")
  end
end

function MultiOsc:phaseControl(objects, branches)
  return function (name, description, index)
    return self:gainControl(objects, branches)(name, description, index, 0, "phase", "[-1,1]")
  end
end

function MultiOsc:onLoadViews(objects, branches)
  local controls = {}
  local views = { expanded = {}, collapsed = { "scope" } }

  controls.scope = OutputScope {
    monitor = self,
    width = 4 * ply
  }

  controls.scope2 = OutputScope {
    monitor = self,
    width = 2 * ply
  }

  local tuneControl = self:tuneControl(objects, branches)
  controls.tune = tuneControl("V/Oct", "Primary V/Oct", 1)
  views.expanded[#views.expanded + 1] = "tune"
  views.tune = { "tune", "scope2" }
  for i = 2, self.oscCount do
    local humanIndex = i - 1
    local name = "tune"..humanIndex
    controls[name] = tuneControl("V/Oct"..humanIndex, "Offset V/Oct "..humanIndex, i)
    views.tune[#views.tune + 1] = name
  end

  local levelControl = self:levelControl(objects, branches)
  controls.level = levelControl("level", "Overall Level", "", 1)
  views.expanded[#views.expanded + 1] = "level"
  views.level = { "level", "scope2" }
  for i = 1, self.oscCount do
    local humanIndex = i - 1
    local name = "level"..humanIndex
    controls[name] = levelControl(name, "Secondary Level "..humanIndex, i, (i == 1 and 1 or 0))
    views.level[#views.level + 1] = name
  end

  if self.isSingleCycle then
    local scanControl = self:scanControl(objects, branches)
    controls.scan = scanControl("scan", "Primary Scan", 1)
    views.expanded[#views.expanded + 1] = "scan"
    views.scan = { "scan", "scope2" }
    for i = 2, self.oscCount do
      local humanIndex = i - 1
      local name = "scan"..humanIndex
      controls[name] = scanControl(name, "Offset Scan "..humanIndex, i)
      views.scan[#views.scan + 1] = name
    end

    local phaseControl = self:phaseControl(objects, branches)
    controls.phase = phaseControl("phase", "Primary Phase", 1)
    views.expanded[#views.expanded + 1] = "phase"
    views.phase = { "phase", "scope2" }
    for i = 2, self.oscCount do
      local humanIndex = i - 1
      local name = "phase"..humanIndex
      controls[name] = phaseControl(name, "Offset Phase "..humanIndex, i)
      views.phase[#views.phase + 1] = name
    end
  end

  controls.f0 = GainBias {
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
  views.expanded[#views.expanded + 1] = "f0"

  controls.sync = Gate {
    button      = "sync",
    description = "Sync",
    branch      = branches.sync,
    comparator  = objects.sync
  }
  views.expanded[#views.expanded + 1] = "sync"

  controls.hSync = Gate {
    button      = "hSync",
    description = "Hard Sync",
    branch      = branches.hardSync,
    comparator  = objects.hardSync
  }
  views.expanded[#views.expanded + 1] = "hSync"

  return controls, views
end

function MultiOsc:serialize()
  local t = Unit.serialize(self)
  local sample = self.sample
  if sample then
    t.sample = SamplePool.serializeSample(sample)
  end
  return t
end

function MultiOsc:deserialize(t)
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

function MultiOsc:onRemove()
  self:setSample(nil)
  Unit.onRemove(self)
end

return function (title, mnemonic, oscType, oscCount)
  local COsc = Class {}
  COsc:include(MultiOsc)

  function COsc:init(args)
    args.title    = title
    args.mnemonic = mnemonic
    args.oscType  = oscType
    args.oscCount = oscCount

    MultiOsc.init(self, args)
  end

  return COsc
end
