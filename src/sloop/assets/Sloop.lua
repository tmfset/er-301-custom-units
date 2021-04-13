local app = app
local sloop = require "sloop.libsloop"
local Class = require "Base.Class"
local Unit = require "Unit"
local SloopView = require "sloop.SloopView"
local GainBias = require "Unit.ViewControl.GainBias"
local Fader = require "Unit.ViewControl.Fader"
local OptionControl = require "Unit.MenuControl.OptionControl"
local MenuHeader = require "Unit.MenuControl.Header"
local Task = require "Unit.MenuControl.Task"
local pool = require "Sample.Pool"
local SamplePoolInterface = require "Sample.Pool.Interface"
local Encoder = require "Encoder"

local Sloop = Class {}
Sloop:include(Unit)

function Sloop:init(args)
  args.title = "Sloop"
  args.mnemonic = "slp"
  self.max = 64;
  Unit.init(self, args)
end

function Sloop:addComparatorControl(name, mode, default)
  local gate = self:addObject(name, app.Comparator())
  gate:setMode(mode)
  self:addMonoBranch(name, gate, "In", gate, "Out")
  if default then
    gate:setOptionValue("State", default)
  end
  return gate
end

function Sloop:addGainBiasControl(name)
  local gb    = self:addObject(name, app.GainBias());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(gb, "Out", range, "In")
  self:addMonoBranch(name, gb, "In", gb, "Out")
  return gb;
end

function Sloop:addParameterAdapterControl(name)
  local pa = self:addObject(name, app.ParameterAdapter())
  self:addMonoBranch(name, pa, "In", pa, "Out")
  return pa
end

function Sloop:onLoadGraph(channelCount)
  local clock   = self:addComparatorControl("clock", app.COMPARATOR_TRIGGER_ON_RISE)
  local engage  = self:addComparatorControl("engage", app.COMPARATOR_TOGGLE, 1)
  local record  = self:addComparatorControl("record", app.COMPARATOR_GATE)
  local overdub = self:addComparatorControl("overdub", app.COMPARATOR_GATE)
  local reset   = self:addComparatorControl("reset", app.COMPARATOR_TRIGGER_ON_RISE)
  local length  = self:addGainBiasControl("length")
  local rLength = self:addGainBiasControl("rLength")
  local feedback = self:addParameterAdapterControl("feedback")
  local resetTo  = self:addParameterAdapterControl("resetTo")

  local head = self:addObject("head", sloop.Sloop(length:getParameter("Bias"), channelCount > 1))
  connect(clock,   "Out", head, "Clock")
  connect(engage,  "Out", head, "Engage")
  connect(record,  "Out", head, "Record")
  connect(overdub, "Out", head, "Overdub")
  connect(reset,   "Out", head, "Reset")
  connect(length,  "Out", head, "Length")
  connect(rLength, "Out", head, "Record Length")

  tie(head, "Feedback", feedback, "Out")
  tie(head, "Reset To", resetTo, "Out")

  for i = 1, channelCount do
    if i > 2 then return end
    connect(self, "In"..i,  head, "In"..i)
    connect(head, "Out"..i, self, "Out"..i)
  end

  self:setInitialBuffer(channelCount)
end

function Sloop:setInitialBuffer(channelCount)
  if self.sample then
    return
  end

  local length = 15
  if not length then
    return
  end

  local sample, status = pool.create {
    channels = channelCount,
    secs     = length,
    root     = "sloop"
  }

  if not sample then
    local Overlay = require "Overlay"
    Overlay.mainFlashMessage("Failed to create buffer.", status)
  end

  self:setSample(sample)
end


function Sloop:setSample(sample)
  if self.sample then self.sample:release(self) end
  self.sample = sample
  if self.sample then self.sample:claim(self) end

  if sample then
    print(sample.pSample:getSizeInBytes())
    print(sample.slices.pSlices:size())

    self.objects.head:setSample(sample.pSample, sample.slices.pSlices)
  else
    self.objects.head:setSample(nil, nil)
  end

  if self.sampleEditor then
    self.sampleEditor:setSample(sample)
  end

  self:notifyControls("setSample", sample)
end

function Sloop:doCreateBuffer()
  local Creator = require "Sample.Pool.Creator"
  local creator = Creator(self.channelCount)

  local task = function(sample)
    if sample then
      local Overlay = require "Overlay"
      Overlay.mainFlashMessage("Attached buffer: %s", sample.name)
      self:setSample(sample)
    end
  end

  creator:subscribe("done",task)
  creator:show()
end

function Sloop:doAttachBufferFromPool()
  local chooser = SamplePoolInterface(self.loadInfo.id, "choose")
  chooser:setDefaultChannelCount(self.channelCount)
  chooser:highlight(self.sample)

  local task = function(sample)
    if sample then
      local Overlay = require "Overlay"
      Overlay.mainFlashMessage("Attached buffer: %s", sample.name)
      self:setSample(sample)
    end
  end

  chooser:subscribe("done", task)
  chooser:show()
end

function Sloop:doAttachSampleFromCard()
  local task = function(sample)
    if sample then
      local Overlay = require "Overlay"
      Overlay.mainFlashMessage("Attached sample: %s",sample.name)
      self:setSample(sample)
    end
  end

  local Pool = require "Sample.Pool"
  Pool.chooseFileFromCard(self.loadInfo.id, task)
end

function Sloop:doDetachBuffer()
  local Overlay = require "Overlay"
  Overlay.mainFlashMessage("Buffer detached.")
  self:setSample()
end

function Sloop:doZeroBuffer()
  local Overlay = require "Overlay"
  Overlay.mainFlashMessage("Buffer zeroed.")
  self.objects.head:zeroBuffer()
end

function Sloop:showSampleEditor()
  if self.sample then
    if self.sampleEditor == nil then
      local SampleEditor = require "Sample.Editor"
      self.sampleEditor = SampleEditor(self, self.objects.head)
      self.sampleEditor:setSample(self.sample)
      self.sampleEditor:setPointerLabel("R")
    end
    self.sampleEditor:show()
  else
    local Overlay = require "Overlay"
    Overlay.mainFlashMessage("You must first select a sample.")
  end
end

function Sloop:describeBuffer()
  if self.sample then
    local bufferName = self.sample:getFilenameForDisplay(30)
    local bufferLength = self.sample:getDurationText()
    return bufferLength..", "..bufferName
  end
end

function Sloop:setCreateBufferMenu(controls, menu)
  local bufferMenuDescription = ""

  if self.sample then
    bufferMenuDescription = "Modify Buffer"
  else
    bufferMenuDescription = "Attach a Buffer!"
  end

  controls.bufferHeader = MenuHeader {
    description = bufferMenuDescription
  }
  menu[#menu + 1] = "bufferHeader"

  controls.createNew = Task {
    description = "New...",
    task        = function() self:doCreateBuffer() end
  }
  menu[#menu + 1] = "createNew"

  controls.attachExisting = Task {
    description = "Pool...",
    task        = function() self:doAttachBufferFromPool() end
  }
  menu[#menu + 1] = "attachExisting"

  controls.selectFromCard = Task {
    description = "Card...",
    task        = function() self:doAttachSampleFromCard() end
  }
  menu[#menu + 1] = "selectFromCard"
end

function Sloop:setModifyBufferMenu(controls, menu)
  controls.editBuffer = Task {
    description = "Edit...",
    task        = function() self:showSampleEditor() end
  }
  menu[#menu + 1] = "editBuffer"

  controls.detachBuffer = Task {
    description = "Detach!",
    task        = function() self:doDetachBuffer() end
  }
  menu[#menu + 1] = "detachBuffer"

  controls.zeroBuffer = Task {
    description = "Zero!",
    task        = function() self:doZeroBuffer() end
  }
  menu[#menu + 1] = "zeroBuffer"
end

function Sloop:onShowMenu(objects, branches)
  local controls, sub, menu = {}, {}, {}

  controls.bufferDescription = MenuHeader {
    description = "At"
  }

  local attached = ""
  if not self.sample then
    self:setCreateBufferMenu(controls, menu)
  else
    attached = " ("..self:describeBuffer()..")"
  end

  controls.recordingHeader = MenuHeader {
    description = "Recording"..attached
  }
  menu[#menu + 1] = "recordingHeader"

  -- controls.recordMode = OptionControl {
  --   description      = "Mode",
  --   descriptionWidth = 2,
  --   option           = self._options.continuousMode.option,
  --   choices          = { "continuous", "manual" },
  --   onUpdate         = self._options.continuousMode.sync
  -- }
  -- menu[#menu + 1] = "recordMode"

  -- controls.recordLength = OptionControl {
  --   description      = "Length",
  --   descriptionWidth = 2,
  --   option           = self._options.fixedRecord.option,
  --   choices          = { "locked", "free" },
  --   onUpdate         = self._options.fixedRecord.sync
  -- }
  -- menu[#menu + 1] = "recordLength"

  -- controls.resetHeader = MenuHeader {
  --   description = "Reset On..."
  -- }
  -- menu[#menu + 1] = "resetHeader"

  -- controls.resetOnDisengage = OptionControl {
  --   description      = "Disengage",
  --   descriptionWidth = 2,
  --   option           = self._options.resetOnDisengage.option,
  --   choices          = { "yes", "no" },
  --   onUpdate         = self._options.resetOnDisengage.sync
  -- }
  -- menu[#menu + 1] = "resetOnDisengage"

  -- controls.resetOnRecord = OptionControl {
  --   description      = "Record",
  --   descriptionWidth = 2,
  --   option           = self._options.resetOnRecord.option,
  --   choices          = { "yes", "no" },
  --   onUpdate         = self._options.resetOnRecord.sync
  -- }
  -- menu[#menu + 1] = "resetOnRecord"


  if self.sample then
    self:setCreateBufferMenu(controls, menu)
    self:setModifyBufferMenu(controls, menu)
  end

  if self.sample then
    sub[#sub + 1] = {
      position = app.GRID5_LINE1,
      justify  = app.justifyLeft,
      text     = "Attached Buffer:"
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
      text     = string.format("+ %s %s %s",self.sample:getChannelText(), self.sample:getSampleRateText(), self.sample:getMemorySizeText())
    }
  else
    sub[#sub + 1] = {
      position = app.GRID5_LINE3,
      justify  = app.justifyCenter,
      text     = "No buffer attached."
    }
  end

  return controls, menu, sub
end

function Sloop.intMap(min, max)
  local map = app.LinearDialMap(min,max)
  map:setSteps(2, 1, 0.25, 0.25)
  map:setRounding(1)
  return map
end

function Sloop:gateView(name, description)
  local Gate = require "Unit.ViewControl.Gate"
  return Gate {
    button      = name,
    description = description,
    branch      = self.branches[name],
    comparator  = self.objects[name]
  }
end

function Sloop:onLoadViews()
  return {
    wave2 = SloopView {
      head  = self.objects.head,
      width = 2 * app.SECTION_PLY
    },
    wave3 = SloopView {
      head  = self.objects.head,
      width = 3 * app.SECTION_PLY
    },
    clock   = self:gateView("clock", "Clock"),
    engage  = self:gateView("engage", "Engage"),
    record  = self:gateView("record", "Record"),
    overdub = self:gateView("overdub", "Overdub"),
    reset   = self:gateView("reset", "Reset"),
    length  = GainBias {
      button        = "length",
      description   = "Length",
      branch        = self.branches.length,
      gainbias      = self.objects.length,
      range         = self.objects.lengthRange,
      gainMap       = self.intMap(-self.max, self.max),
      biasMap       = self.intMap(1, self.max),
      biasPrecision = 0,
      initialBias   = 4
    },
    rLength  = GainBias {
      button        = "rLength",
      description   = "Record Length",
      branch        = self.branches.rLength,
      gainbias      = self.objects.rLength,
      range         = self.objects.rLengthRange,
      gainMap       = self.intMap(-self.max, self.max),
      biasMap       = self.intMap(1, self.max),
      biasPrecision = 0,
      initialBias   = 4
    },
    feedback   = GainBias {
      button        = "feedback",
      description   = "Feedback",
      branch        = self.branches.feedback,
      gainbias      = self.objects.feedback,
      range         = self.objects.feedback,
      biasMap       = Encoder.getMap("[0,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 1
    },
    resetTo   = GainBias {
      button        = "resetTo",
      description   = "Reset To",
      branch        = self.branches.resetTo,
      gainbias      = self.objects.resetTo,
      range         = self.objects.resetTo,
      gainMap       = self.intMap(-self.max, self.max),
      biasMap       = self.intMap(0, self.max),
      biasPrecision = 0,
      initialBias   = 0
    },
    fade   = Fader {
      button      = "fade",
      description = "Fade",
      param       = self.objects.head:getParameter("Fade"),
      map         = Encoder.getMap("[0,1]"),
      units       = app.unitSecs,
      precision   = 3,
      initial     = 0.005
    },
    fadeIn   = Fader {
      button      = "fadeIn",
      description = "Fade In",
      param       = self.objects.head:getParameter("Fade In"),
      map         = Encoder.getMap("[0,10]"),
      units       = app.unitSecs,
      precision   = 2,
      initial     = 0.01
    },
    fadeOut   = Fader {
      button      = "fadeOut",
      description = "Fade Out",
      param       = self.objects.head:getParameter("Fade Out"),
      map         = Encoder.getMap("[0,10]"),
      units       = app.unitSecs,
      precision   = 2,
      initial     = 0.1
    },
    through   = Fader {
      button      = "through",
      description = "Through",
      param       = self.objects.head:getParameter("Through"),
      map         = Encoder.getMap("[0,1]"),
      units       = app.unitNone,
      precision   = 2,
      initial     = 1
    }
  }, {
    expanded  = { "clock", "reset", "record", "overdub", "through" },

    clock     = { "clock",    "wave2", "engage", "length" },
    reset     = { "reset",    "wave2", "resetTo", "fade" },
    record    = { "record",   "wave2", "fadeIn", "fadeOut" },
    overdub   = { "overdub",  "wave2", "feedback", "fadeIn", "fadeOut" },
    through   = { "through",  "wave3" },

    collapsed = { "wave3" },
  }
end

return Sloop
