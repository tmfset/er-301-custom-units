-- luacheck: globals app connect
local app = app
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"
local Encoder = require "Encoder"
local SamplePool = require "Sample.Pool"
local RecordingView = require "builtins.Looper.RecordingView"
local SamplePoolInterface = require "Sample.Pool.Interface"
local Task = require "Unit.MenuControl.Task"
local MenuHeader = require "Unit.MenuControl.Header"
local ply = app.SECTION_PLY

local Sloop = Class{}
Sloop:include(Unit)

local maxSteps = 32
local initialSteps = 4

function Sloop:init(args)
  args.title    = "Sloop"
  args.mnemonic = "slp"

  Unit.init(self, args)
end

-- Create a memoized constant value to be used as an output.
function Sloop:mConst(value)
  self.constants = self.constants or {}

  if self.constants[value] == nil then
    local const = self:createObject("Constant", "constant"..value)
    const:hardSet("Value", value)

    self.constants[value] = const
  end

  return self.constants[value]
end

-- Add together two outputs.
function Sloop:sum(left, right, suffix)
  local sum = self:createObject("Sum", "sum"..suffix)
  connect(left, "Out", sum, "Left")
  connect(right, "Out", sum, "Right")
  return sum
end

-- Multiply two outputs.
function Sloop:mult(left, right, suffix)
  local mult = self:createObject("Multiply", "vca"..suffix)
  connect(left, "Out", mult, "Left")
  connect(right, "Out", mult, "Right")
  return mult
end

-- Constant gain bias.
function Sloop:cGainBias(gain, bias, suffix)
  local gb = self:createObject("GainBias", "gainBias"..suffix)
  gb:hardSet("Gain", gain)
  gb:hardSet("Bias", bias)
  return gb
end

-- Inverted gate signal.
function Sloop:iGate(gate, suffix)
  local gb = self:cGainBias(-1, 1, "iGateGb"..suffix)
  connect(gate, "Out", gb, "In")
  return gb
end

-- A basic latch with reset.
function Sloop:latch(input, reset, suffix)
  local name = function (str) return "Latch"..str..suffix end

  local gate = self:createObject("Counter", name("Gate"))
  gate:hardSet("Start", 0)
  gate:hardSet("Finish", 1)
  gate:hardSet("Step Size", 1)
  gate:hardSet("Gain", 1)

  local invGate     = self:iGate(gate, name("InvGate"))
  local inputSignal = self:mult(input, invGate, name("InputSignal"))

  connect(inputSignal, "Out", gate, "In")
  connect(reset,       "Out", gate, "Reset")

  return gate
end

-- A clocked latch. When gate is high trigger the latch on the clock. When gate
-- is low trigger the reset on the clock.
function Sloop:clockLatch(clock, gate, suffix)
  local name = function (str) return "ClockLatch"..str..suffix end

  local input   = self:mult(clock, gate, name("Input"))
  local invGate = self:iGate(gate, name("InvGate"))
  local reset   = self:mult(clock, invGate, name("Reset"))

  return self:latch(input, reset, name("Latch"))
end

-- Step values for use in the count latch.
function Sloop:vStepLength(length, suffix)
  local scale  = 1 / maxSteps
  local last   = self:sum(length, self:mConst(-1), "lastStep"..suffix)
  local scaled = self:mult(last, self:mConst(scale), "lastStepScaled"..suffix)
  return { scale = scale, last = last, scaled = scaled }
end

-- A latch that only opens after a variable number of inputs.
function Sloop:countLatch(steps, input, reset, suffix)
  local name = function (str) return "CountLatch"..str..suffix end

  local finish = self:createObject("ParameterAdapter", name("Finish"))
  finish:hardSet("Gain", 1)
  connect(steps.last, "Out", finish, "In")

  local center = self:createObject("ParameterAdapter", name("Center"))
  center:hardSet("Gain", 1)
  connect(steps.scaled, "Out", center, "In")

  local counter = self:createObject("Counter", name("Counter"))
  counter:setOptionValue("Processing Rate", 2) -- Sample Rate
  counter:hardSet("Start", 0)
  counter:hardSet("Step Size", 1)
  counter:hardSet("Gain", steps.scale)
  tie(counter, "Finish", finish, "Out")

  local gate = self:createObject("BumpMap", name("Gate"))
  gate:hardSet("Width", steps.scale)
  gate:hardSet("Height", 1)
  gate:hardSet("Fade", 0)
  tie(gate, "Center", center, "Out")
  connect(counter, "Out", gate, "In")

  local invGate     = self:iGate(gate, name("InvReset"))
  local inputSignal = self:mult(input, invGate, name("InputSignal"))

  connect(inputSignal, "Out", counter, "In")
  connect(reset,       "Out", counter, "Reset")

  return gate
end

function Sloop:createControl(type, name)
  local control = self:createObject(type, name)
  local controlRange = self:createObject("MinMax", name.."Range")
  connect(control, "Out", controlRange, "In")
  self:createMonoBranch(name, control, "In", control, "Out")
  return control
end

function Sloop:createComparatorControl(name, mode)
  local gate = self:createObject("Comparator", name)
  gate:setMode(mode)
  self:createMonoBranch(name, gate, "In", gate, "Out")
  return gate
end

function Sloop:createToggleControl(name)
  return self:createComparatorControl(name, 1)
end

function Sloop:createGateControl(name)
  return self:createComparatorControl(name, 2)
end

function Sloop:createTriggerControl(name)
  return self:createComparatorControl(name, 3)
end

function Sloop:createControls()
  return {
    length   = self:createControl("GainBias", "length"),
    rLength  = self:createControl("GainBias", "rLength"),
    clock    = self:createTriggerControl("clock"),
    engage   = self:createToggleControl("engage"),
    record   = self:createTriggerControl("record"),
    reset    = self:createTriggerControl("reset"),
    through  = self:createControl("GainBias", "through"),
    feedback = self:createControl("GainBias", "feedback"),
    fadeIn   = self:createControl("ParameterAdapter", "fadeIn"),
    fadeOut  = self:createControl("ParameterAdapter", "fadeOut")
  }
end

function Sloop:onLoadGraph(channelCount)
  local controls = self:createControls()
  local steps    = self:vStepLength(controls.length, "Steps")
  local rSteps   = self:vStepLength(controls.rLength, "RecordSteps")

  local engageLatch  = self:clockLatch(controls.clock, controls.engage, "EngageLatch")
  local engagedClock = self:mult(controls.clock, engageLatch, "EngagedClock")

  local resetGate   = self:createObject("Comparator", "ResetGate")
  local resetSignal = self:mult(engagedClock, resetGate, "ResetSignal")
  resetGate:setGateMode()

  local manualResetLatch  = self:latch(controls.reset,  engagedClock, "ManualResetLatch")
  local endOfCycleLatch   = self:countLatch(steps, engagedClock, resetSignal, "EndOfCycleLatch")
  local resetGateSum      = self:sum(manualResetLatch, endOfCycleLatch, "ResetGateSum")
  connect(resetGateSum, "Out", resetGate, "In")

  local punchInSignal     = self:createObject("Multiply", "PunchInSignal")
  local manualRecordLatch = self:latch(controls.record, punchInSignal, "ManualRecordLatch")
  connect(manualRecordLatch, "Out", punchInSignal, "Left")
  connect(engagedClock,      "Out", punchInSignal, "Right")

  local punchOutLatch  = self:countLatch(rSteps, engagedClock, punchInSignal, "PunchOutLatch")
  local punchOutSignal = self:mult(engagedClock, punchOutLatch, "PunchOutSignal")
  local punchLatch     = self:latch(punchInSignal, punchOutSignal, "PunchLatch")

  local recordInSlew  = self:createObject("SlewLimiter", "RecordInSlew")
  local recordOutSlew = self:createObject("SlewLimiter", "RecordLevel")
  local recordLevel   = recordOutSlew
  recordInSlew:setOptionValue("Direction", 1) -- Up
  recordOutSlew:setOptionValue("Direction", 3) -- Down
  tie(recordInSlew, "Time", controls.fadeIn, "Out")
  tie(recordOutSlew, "Time", controls.fadeOut, "Out")
  connect(punchLatch, "Out", recordInSlew, "In")
  connect(recordInSlew, "Out", recordOutSlew, "In")

  local feedbackDiff   = self:sum(self:mConst(-1), controls.feedback, "FeedbackDiff")
  local feedbackOffset = self:mult(punchLatch, feedbackDiff, "FeedbackOffset")
  local feedback       = self:sum(self:mConst(1), feedbackOffset, "Feedback")

  local head = self:createObject("FeedbackLooper", "head", channelCount)
  connect(feedback,    "Out", head, "Feedback")
  connect(engageLatch, "Out", head, "Engage")
  connect(resetSignal, "Out", head, "Reset")
  connect(recordLevel, "Out", head, "Punch")

  local actualRecordLevel = self:mult(recordLevel, engageLatch, "ActualRecordLevel")
  local invRecordLevel = self:iGate(actualRecordLevel, "InvRecordLevel")

  local throughLeft      = self:createObject("Multiply", "ThroughLeft")
  local throughLeftTotal = self:mult(throughLeft, controls.through, "ThroughLeftTotal")
  connect(self,           "In1", throughLeft, "Left")
  connect(invRecordLevel, "Out", throughLeft, "Right")

  local mixLeft = self:createObject("Sum", "MixLeft")
  connect(throughLeftTotal, "Out",      mixLeft, "Left")
  connect(head,             "Left Out", mixLeft, "Right")

  local inputLeft = self:createObject("Multiply", "InputLeft")
  connect(self,        "In1", inputLeft, "Left")
  connect(recordLevel, "Out", inputLeft, "Right")

  connect(inputLeft, "Out", head, "Left In")
  connect(mixLeft,   "Out", self, "Out1")

  if channelCount > 1 then
    local throughRight      = self:createObject("Multiply", "ThroughRight")
    local throughRightTotal = self:mult(throughRight, controls.through, "ThroughRightTotal")
    connect(self,           "In2", throughRight, "Left")
    connect(invRecordLevel, "Out", throughRight, "Right")

    local mixRight = self:createObject("Sum", "MixRight")
    connect(throughRightTotal, "Out",       mixRight, "Left")
    connect(head,              "Right Out", mixRight, "Right")

    local inputRight = self:createObject("Multiply", "InputRight")
    connect(self,        "In2", inputRight, "Left")
    connect(recordLevel, "Out", inputRight, "Right")

    connect(inputRight, "Out", head, "Right In")
    connect(mixRight,   "Out", self, "Out2")
  end
end

function Sloop:setSample(sample)
  if self.sample then
    self.sample:release(self)
  end

  self.sample = sample

  if sample then
    self.sample:claim(self)
    self.objects.head:setSample(sample.pSample)
  else
    self.objects.head:setSample(nil)
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

function Sloop:onLoadMenu(objects, branches)
  local controls, sub, menu = {}, {}, {}

  local bufferMenuDescription = "Buffer Menu"
  if self.sample then
    local bufferName = self.sample:getFilenameForDisplay(24)
    local bufferLength = self.sample:getDurationText()
    bufferMenuDescription = bufferMenuDescription.." ("..bufferName..", "..bufferLength..")"
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

  if self.sample then
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

function Sloop:onLoadViews(objects, branches)
  local controls, views = {}, {
    expanded  = { "clock", "engage", "reset", "record", "through", "steps", "rSteps", "feedback", "fadeIn", "fadeOut" },
    collapsed = { "wave5" },

    clock     = { "wave3", "clock", "engage" },
    engage    = { "wave3", "clock", "engage" },

    reset     = { "wave3", "clock", "reset", "steps" },
    steps     = { "wave3", "clock", "reset", "steps" },

    rSteps    = { "wave3", "record", "rSteps" },
    record    = { "wave3", "record", "rSteps" },
    feedback  = { "wave3", "record", "feedback" },
    fadeIn    = { "wave2", "record", "fadeIn", "fadeOut" },
    fadeOut   = { "wave2", "record", "fadeIn", "fadeOut" }
  }

  controls.wave2 = RecordingView {
    head  = objects.head,
    width = 2 * ply
  }

  controls.wave3 = RecordingView {
    head  = objects.head,
    width = 3 * ply
  }

  controls.wave5 = RecordingView {
    head  = objects.head,
    width = 5 * ply
  }

  controls.steps = GainBias {
    button        = "steps",
    description   = "Cycle Length",
    branch        = branches.length,
    gainbias      = objects.length,
    range         = objects.lengthRange,
    biasMap       = Encoder.getMap("int[1,"..maxSteps.."]"),
    biasPrecision = 0,
    initialBias   = initialSteps,
  }

  controls.rSteps = GainBias {
    button        = "rSteps",
    description   = "Record Length",
    branch        = branches.rLength,
    gainbias      = objects.rLength,
    range         = objects.rLengthRange,
    biasMap       = Encoder.getMap("int[1,"..maxSteps.."]"),
    biasPrecision = 0,
    initialBias   = initialSteps,
  }

  controls.clock = Gate {
    button      = "clock",
    description = "Clock",
    branch      = branches.clock,
    comparator  = objects.clock
  }

  controls.engage = Gate {
    button      = "engage",
    description = "Engage",
    branch      = branches.engage,
    comparator  = objects.engage
  }

  controls.record = Gate {
    button      = "record",
    description = "Record",
    branch      = branches.record,
    comparator  = objects.record
  }

  controls.reset = Gate {
    button      = "reset",
    description = "Reset",
    branch      = branches.reset,
    comparator  = objects.reset
  }

  controls.through = GainBias {
    button        = "through",
    description   = "Through Level",
    branch        = branches.through,
    gainbias      = objects.through,
    range         = objects.throughRange,
    biasMap       = Encoder.getMap("[0,1]"),
    biasUnits     = app.unitNone,
    biasPrecision = 1,
    initialBias   = 1
  }

  controls.feedback = GainBias {
    button        = "rFdbk",
    description   = "Feedback",
    branch        = branches.feedback,
    gainbias      = objects.feedback,
    range         = objects.feedbackRange,
    biasMap       = Encoder.getMap("[0,1]"),
    biasUnits     = app.unitNone,
    biasPrecision = 1,
    initialBias   = 1
  }

  controls.fadeIn = GainBias {
    button        = "rFdIn",
    description   = "Record Fade In Time",
    branch        = branches.fadeIn,
    gainbias      = objects.fadeIn,
    range         = objects.fadeInRange,
    biasMap       = Encoder.getMap("[0,10]"),
    biasUnits     = app.unitSecs,
    biasPrecision = 2,
    initialBias   = 0.01
  }

  controls.fadeOut = GainBias {
    button        = "rFdOut",
    description   = "Record Fade Out Time",
    branch        = branches.fadeOut,
    gainbias      = objects.fadeOut,
    range         = objects.fadeOutRange,
    biasMap       = Encoder.getMap("[0,10]"),
    biasUnits     = app.unitSecs,
    biasPrecision = 2,
    initialBias   = 0.1
  }

  return controls, views
end

function Sloop:serialize()
  local t = Unit.serialize(self)
  if self.sample then
    t.sample = SamplePool.serializeSample(self.sample)
    t.samplePosition = self.objects.head:getPosition()
  end
  return t
end

function Sloop:deserialize(t)
  Unit.deserialize(self,t)
  if t.sample then
    local sample = SamplePool.deserializeSample(t.sample)
    if sample then
      self:setSample(sample)
      if t.samplePosition then
        self.objects.head:setPosition(t.samplePosition)
      end
    else
      local Utils = require "Utils"
      app.log("%s:deserialize: failed to load sample.",self)
      Utils.pp(t.sample)
    end
  end
end

function Sloop:onRemove()
  self:setSample(nil)
  Unit.onRemove(self)
end

return Sloop
