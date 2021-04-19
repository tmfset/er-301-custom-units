local app = app
local sloop = require "sloop.libsloop"
local Class = require "Base.Class"
local Unit = require "Unit"
local SloopView = require "sloop.SloopView"
local TaskListControl = require "sloop.TaskListControl"
local OptionControl = require "sloop.SloopOptionControl"
local FlagSelect = require "sloop.SloopFlagControl"
local SloopHeader = require "sloop.SloopHeader"
local GainBias = require "Unit.ViewControl.GainBias"
local Fader = require "Unit.ViewControl.Fader"
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

function Sloop:addBranchlessComparatorControl(name, mode, default)
  local gate = self:addObject(name, app.Comparator())
  gate:setMode(mode)
  if default then
    gate:setOptionValue("State", default)
  end
  return gate
end

function Sloop:addComparatorControl(name, mode, default)
  local gate = self:addBranchlessComparatorControl(name, mode, default);
  self:addMonoBranch(name, gate, "In", gate, "Out")
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
  local clock     = self:addComparatorControl("clock", app.COMPARATOR_TRIGGER_ON_RISE)
  local engage    = self:addComparatorControl("engage", app.COMPARATOR_TOGGLE, 1)
  local record    = self:addComparatorControl("record", app.COMPARATOR_GATE)
  local overdub   = self:addComparatorControl("overdub", app.COMPARATOR_GATE)
  local reset     = self:addBranchlessComparatorControl("reset", app.COMPARATOR_TRIGGER_ON_RISE)
  local length    = self:addGainBiasControl("length")
  local dubLength = self:addGainBiasControl("dubLength")
  local feedback  = self:addParameterAdapterControl("feedback")
  local resetTo   = self:addParameterAdapterControl("resetTo")

  local head = self:addObject("head", sloop.Sloop(
    length:getParameter("Bias"),
    dubLength:getParameter("Bias"),
    channelCount > 1
  ))

  connect(clock,     "Out", head, "Clock")
  connect(engage,    "Out", head, "Engage")
  connect(record,    "Out", head, "Record")
  connect(overdub,   "Out", head, "Overdub")
  connect(reset,     "Out", head, "Reset")
  connect(length,    "Out", head, "Length")
  connect(dubLength, "Out", head, "Overdub Length")

  self:addMonoBranch(reset:name(), reset, "In", head, "Reset Out")

  tie(head, "Feedback", feedback, "Out")
  tie(head, "Reset To", resetTo, "Out")

  for i = 1, channelCount do
    if i > 2 then return end
    connect(self, "In"..i,  head, "In"..i)
    connect(head, "Out"..i, self, "Out"..i)
  end
end


function Sloop:serialize()
  local t = Unit.serialize(self)

  local sample = self.sample
  if sample then
    t.sample = pool.serializeSample(sample)
  end

  local marks = self.objects.head:getClockMarks()
  t.clockMarks = {}
  for i = 1, marks:size() do
    t.clockMarks[i] = marks:get(i - 1)
  end

  t.currentStep = self.objects.head:currentStep()

  return t
end

function Sloop:deserialize(t)
  Unit.deserialize(self, t)

  if t.sample then
    local sample = pool.deserializeSample(t.sample)
    if sample then
      self:setSample(sample)
    else
      local Utils = require "Utils"
      app.logError("%s:deserialize: failed to load sample.", self)
      Utils.pp(t.sample)
    end
  end

  local marks = self.objects.head:getClockMarks()
  local saved = t.clockMarks or {}
  for i = 1, #saved do
    marks:set(i - 1, saved[i])
  end

  self.objects.head:setCurrentStep(t.currentStep or 0)
end


function Sloop:setSample(sample)
  if self.sample then self.sample:release(self) end
  self.sample = sample
  if self.sample then self.sample:claim(self) end

  if sample then
    self.objects.head:setSample(sample.pSample, sample.slices.pSlices)
  else
    self.objects.head:setSample(nil, nil)
  end

  if self.sampleEditor then
    self.sampleEditor:setSample(sample)
  end

  self:notifyControls("setSample", sample, self:describeBuffer())
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
      self:setSample(sample)
      local Overlay = require "Overlay"
      Overlay.mainFlashMessage("Attached buffer: %s", sample.name)
    end
  end

  chooser:subscribe("done", task)
  chooser:show()
end

function Sloop:doAttachSampleFromCard()
  local task = function(sample)
    if sample then
      self:setSample(sample)
      local Overlay = require "Overlay"
      Overlay.mainFlashMessage("Attached sample: %s",sample.name)
    end
  end

  local Pool = require "Sample.Pool"
  Pool.chooseFileFromCard(self.loadInfo.id, task)
end

function Sloop:doDetachBuffer()
  self:setSample(nil)
  local Overlay = require "Overlay"
  Overlay.mainFlashMessage("Buffer detached.")
end

function Sloop:doZeroBuffer()
  local Overlay = require "Overlay"
  self.objects.head:zeroBuffer()
  Overlay.mainFlashMessage("Buffer zeroed.")
end

function Sloop:doClearSlices()
  local n = self.objects.head:clearSlices()
  local Overlay = require "Overlay"
  Overlay.mainFlashMessage("Deleted "..n.." slices.")
end

function Sloop:doCreateSlices()
  local n = self.objects.head:createSlices()
  local Overlay = require "Overlay"
  Overlay.mainFlashMessage("Created "..n.." slices.")
end

function Sloop:showSampleEditor()
  if self.sample then
    if self.sampleEditor == nil then
      local SampleEditor = require "Sample.Editor"
      self.sampleEditor = SampleEditor(self, self.objects.head)
      self.sampleEditor:setSample(self.sample)
      self.sampleEditor:setPointerLabel("P")
    end
    self.sampleEditor:show()
  else
    local Overlay = require "Overlay"
    Overlay.mainFlashMessage("You must first select a sample.")
  end
end

function Sloop:describeBuffer(length)
  if self.sample then
    local bufferName = self.sample:getFilenameForDisplay(length or 18) -- 25, 18
    local bufferLength = self.sample:getDurationText()
    return bufferLength..", "..bufferName
  end

  return "Nothing attached"
end


function Sloop:onShowMenu(objects, branches)
  local controls, sub, menu = {}, {}, {}

  controls.header = SloopHeader {}
  menu[#menu + 1] = "header"

  controls.attachBuffer = TaskListControl {
    description = "Attach buffer",
    descriptionWidth = 3,
    tasks = {
      {
        description = "New...",
        task        = function() self:doCreateBuffer() end
      },
      {
        description = "Pool...",
        task        = function() self:doAttachBufferFromPool() end
      },
      {
        description = "Card...",
        task        = function() self:doAttachSampleFromCard() end
      }
    }
  }
  menu[#menu + 1] = "attachBuffer"

  if self.sample then
    controls.modifyBuffer = TaskListControl {
      description = "Modify buffer",
      descriptionWidth = 3,
      tasks = {
        {
          description = "Edit...",
          task        = function() self:showSampleEditor() end
        },
        {
          description = "Detach!",
          task        = function() self:doDetachBuffer() end
        },
        {
          description = "Zero!",
          task        = function() self:doZeroBuffer() end
        }
      }
    }
    menu[#menu + 1] = "modifyBuffer"

    controls.modifySlices = TaskListControl {
      description = "Modify slices",
      descriptionWidth = 4,
      tasks = {
        {
          description = "Create!",
          task        = function() self:doCreateSlices() end
        },
        {
          description = "Clear!",
          task        = function() self:doClearSlices() end
        }
      }
    }
    menu[#menu + 1] = "modifySlices"
  end

  controls.configHeader = SloopHeader {
    description = "Configuration:"
  }
  menu[#menu + 1] = "configHeader"

  controls.dubLength = OptionControl {
    description      = "Overdub length",
    descriptionWidth = 3,
    option           = objects.head:getOption("Lock Overdub Length"),
    choices          = { "locked", "free" },
    choicesPadding   = 1
  }
  menu[#menu + 1] = "dubLength"

  controls.resetMode = OptionControl {
    description      = "Manual reset mode",
    descriptionWidth = 3,
    option           = objects.head:getOption("Reset Mode"),
    choices          = { "jump", "step", "random" }
  }
  menu[#menu + 1] = "resetMode"

  controls.resetOn = FlagSelect {
    description      = "Auto reset on...",
    descriptionWidth = 3,
    option           = objects.head:getOption("Reset Flags"),
    flags            = { "disengage", "eoc", "overdub" }
  }
  menu[#menu + 1] = "resetOn"

  controls.sliceMode = FlagSelect {
    description      = "Auto slice on...",
    descriptionWidth = 3,
    option           = objects.head:getOption("Slice Mode"),
    flags            = { "clock", "reset" },
    flagsPadding     = 1
  }
  menu[#menu + 1] = "sliceMode"

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
      parent = self,
      head  = self.objects.head,
      width = 2 * app.SECTION_PLY,
      description = self:describeBuffer()
    },
    wave3 = SloopView {
      parent = self,
      head  = self.objects.head,
      width = 3 * app.SECTION_PLY,
      description = self:describeBuffer()
    },
    clock   = self:gateView("clock", "Clock"),
    engage  = self:gateView("engage", "Engage"),
    record  = self:gateView("record", "Continuous Record"),
    overdub = self:gateView("overdub", "Overdub"),
    reset   = self:gateView("reset", "Manual Reset"),
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
    dubLength  = GainBias {
      button        = "olength",
      description   = "Overdub Length",
      branch        = self.branches.dubLength,
      gainbias      = self.objects.dubLength,
      range         = self.objects.dubLengthRange,
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
      biasMap       = self.intMap(-self.max, self.max),
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
    expanded  = { "clock", "reset", "record", "overdub" },

    clock     = { "clock",   "wave2", "engage", "length" },
    reset     = { "reset",   "wave2", "resetTo", "fade" },
    record    = { "record",  "wave2", "length", "dubLength" },
    overdub   = { "overdub", "wave2", "feedback", "dubLength" },

    collapsed = { "wave3" },
  }
end

function Sloop:onRemove()
  self:setSample(nil)
  Unit.onRemove(self)
end

return Sloop
