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
local OptionControl = require "Unit.MenuControl.OptionControl"
local Task = require "Unit.MenuControl.Task"
local MenuHeader = require "Unit.MenuControl.Header"
local ply = app.SECTION_PLY

local config = require "Sloop.defaults"

local Sloop = Class{}
Sloop:include(Unit)

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

function Sloop:addToMix(mix, index, input)
  connect(input, "Out", mix, "In"..index)
  mix:hardSet("Gain"..index, 1)
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

function Sloop:clip(input, name)
  local clip = self:createObject("Clipper", name)
  connect(input, "Out", clip, "In")
  return clip
end

function Sloop:logicalAnd(left, right, suffix)
  return self:mult(left, right, suffix)
end

function Sloop:logicalOr(left, right, suffix)
  local sum = self:sum(left, right, "Or"..suffix)
  return self:clip(sum, "LogicalOrClip"..suffix)
end

function Sloop:logicalNot(input, suffix)
  local gb = self:cGainBias(-1, 1, "Not"..suffix)
  connect(input, "Out", gb, "In")
  return gb
end

-- Constant gain bias.
function Sloop:cGainBias(gain, bias, suffix)
  local gb = self:createObject("GainBias", "gainBias"..suffix)
  gb:hardSet("Gain", gain)
  gb:hardSet("Bias", bias)
  return gb
end

function Sloop:cGate(input, suffix)
  local gate = self:createObject("Comparator", "ConstantGate"..suffix)
  connect(input, "Out", gate, "In")
  gate:setGateMode()
  return gate
end

function Sloop:cTrig(input, suffix)
  local trig = self:createObject("Comparator", "ConstantTrig"..suffix)
  connect(input, "Out", trig, "In")
  return trig
end

function Sloop:toggle(name)
  local gate = self:createObject("Comparator", name)
  gate:setToggleMode()
  return gate
end

-- A simple counter.
function Sloop:newCounter(start, finish, stepSize, gain, name)
  local counter = self:createObject("Counter", name)
  counter:setOptionValue("Processing Rate", 2) -- Sample Rate
  counter:hardSet("Start", start)
  counter:hardSet("Finish", finish)
  counter:hardSet("Step Size", stepSize)
  counter:hardSet("Gain", gain)
  counter:hardSet("Value", start)
  return counter
end

function Sloop:latch(input, reset, suffix)
  local name = function (str) return "Latch"..str..suffix end

  local gate    = self:toggle(name("Gate"))
  local notGate = self:logicalNot(gate, name("NotGate"))

  local onSignal  = self:mult(notGate, input, name("On"))
  local offSignal = self:mult(gate, reset, name("Off"))

  local inputSignal = self:sum(onSignal, offSignal, name("Input"))
  connect(inputSignal, "Out", gate, "In")

  return gate
end

function Sloop:vFinishCounter(start, finish, stepSize, gain, suffix)
  local name = function (str) return "VFinishCounter"..str..suffix end

  local counter = self:createObject("Counter", name("Count"))
  counter:setOptionValue("Processing Rate", 2) -- Sample Rate
  counter:hardSet("Start", start)
  counter:hardSet("Step Size", stepSize)
  counter:hardSet("Gain", gain)

  local finishAdapter = self:createObject("ParameterAdapter", name("Finish"))
  finishAdapter:hardSet("Gain", 1)
  connect(finish, "Out", finishAdapter, "In")
  tie(counter, "Finish", finishAdapter, "Out")

  return counter
end

-- A clocked gate. When `gate` is high, output goes high on the next clock
-- tick. When `gate` is low, output goes low on the next clock tick.
function Sloop:clockGate(clock, gate, suffix)
  local name = function (str) return "ClockGate"..str..suffix end

  local input   = self:mult(clock, gate, name("Input"))
  local invGate = self:logicalNot(gate, name("InvGate"))
  local reset   = self:mult(clock, invGate, name("Reset"))

  return self:latch(input, reset, name("Latch"))
end

function Sloop:countDownGate(clock, length, reset, suffix)
  local name = function (str) return "CountDownGate"..str..suffix end

  local steps   = self:sum(length, self:mConst(-1), name("Steps"))
  local counter = self:vFinishCounter(0, steps, -1, 0.25, name("CountDown"))
  local wait    = self:cGate(counter, name("Gate"))
  local stop    = self:logicalNot(wait, name("NotGate"))
  local input   = self:mult(wait, clock, name("Input"))

  connect(input, "Out", counter, "In")
  connect(reset, "Out", counter, "Reset")

  return stop
end

function Sloop:countDown(clock, length, reset, suffix)
  local name = function (str) return "CountDown"..str..suffix end

  local gate = self:countDownGate(clock, length, reset, name("Gate"))
  return self:mult(clock, gate, name("Output"))
end

function Sloop:clockCountGate(clock, length, gate, bypass, suffix)
  local name = function (str) return "ClockCountGate"..str..suffix end

  local notGate = self:logicalNot(gate, name("NotGate"))

  local startTrig   = self:cTrig(gate, name("StartTrigger"))
  local startLatch  = self:latch(startTrig, clock, name("StartLatch"))
  local startSignal = self:mult(startLatch, clock, name("StartSignal"))

  local countDownEnd  = self:countDownGate(clock, length, startSignal, name("CountDownEnd"))
  local counting = self:latch(startSignal, countDownEnd, name("Counting"))
  local notCounting = self:logicalNot(counting, name("NotCounting"))
  local notCountingBypass = self:logicalOr(notCounting, bypass, name("NotCountingBypass"))

  local disabled = self:mult(notCountingBypass, notGate, name("Disabled"))
  local stopSignal = self:mult(disabled, clock, name("StopSignal"))

  local output = self:latch(startSignal, stopSignal, name("Output"))

  return {
    start = startSignal,
    gate  = output,
    last  = self:mult(output, disabled, name("Last")),
    stop  = self:mult(output, stopSignal, name("ActualStop"))
  }
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
  self._controls = {
    length   = self:createControl("GainBias", "length"),
    rLength  = self:createControl("GainBias", "rLength"),
    clock    = self:createTriggerControl("clock"),
    engage   = self:createToggleControl("engage"),
    record   = self:createGateControl("record"),
    reset    = self:createTriggerControl("reset"),
    through  = self:createControl("GainBias", "through"),
    feedback = self:createControl("GainBias", "feedback"),
    fadeIn   = self:createControl("ParameterAdapter", "fadeIn"),
    fadeOut  = self:createControl("ParameterAdapter", "fadeOut"),

    resetOnDisengageGate   = self:createObject("Constant", "ResetOnDisengage"),
    resetOnDisengageOption = app.Option("EnableResetOnDisengage"),

    resetOnRecordGate      = self:createObject("Constant", "ResetOnRecord"),
    resetOnRecordOption    = app.Option("EnableResetOnRecord"),

    fixedRecordOption      = app.Option("EnableFixedRecordLength"),

    continuousModeGate     = self:createObject("Constant", "ContinuousMode"),
    continuousModeOption   = app.Option("EnableContinuousMode"),
    continuousCount        = self:newCounter(1, config.maxSteps, 1, 1 / config.maxSteps, "ContinuousCount")
  }

  -- Turn on engage by default.
  self._controls.engage:setOptionValue("State", config.startEngaged)

  self._controls.resetOnDisengageOption:set(config.defaultResetOnDisengage)
  self._controls.resetOnRecordOption:set(config.defaultResetOnRecord)
  self._controls.fixedRecordOption:set(config.defaultLengthMode)
  self._controls.continuousModeOption:set(config.defaultMode)

  return self._controls
end

function Sloop:onLoadGraph(channelCount)
  local controls = self:createControls()

  local engaged         = self:clockGate(controls.clock, controls.engage, "Engaged")
  local disengaged      = self:logicalNot(engaged, "Disengaged")
  local disengageSignal = self:cTrig(disengaged, "DisengageSignal")
  local engagedClock    = self:mult(controls.clock, controls.engage, "EngagedClock")

  local punch     = self:clockCountGate(engagedClock, controls.rLength, controls.record, controls.continuousModeGate, "Punch")
  local punchSlew = self:slew(punch.gate, controls.fadeIn, controls.fadeOut, "Punch")

  local resetOnRecord    = self:logicalOr(controls.resetOnRecordGate, controls.continuousModeGate, "ResetOnRecord")
  local resetByForceGate = self:latch(controls.reset, engagedClock, "ManualResetLatch")

  local reset = self:createObject("Mixer", "Reset", 5)
  self:addToMix(reset, 1, self:mult(resetOnRecord, punch.start, "ResetonRecordSignal"))
  self:addToMix(reset, 2, self:mult(resetByForceGate, engagedClock, "ResetGate"))
  self:addToMix(reset, 3, self:mult(disengageSignal, controls.resetOnDisengageGate, "ResetOnDisengageSignal"))
  self:addToMix(reset, 4, self:mult(punch.stop, controls.continuousModeGate, "ContinuousReset"))

  local continuousRecord    = self:mult(punch.gate, controls.continuousModeGate, "ContinuousRecord")
  local notContinuousRecord = self:logicalNot(continuousRecord, "NotContinuousRecord")
  local naturalEndOfCycle   = self:countDown(engagedClock, controls.length, reset, "NaturalEndOfCycle")
  self:addToMix(reset, 5, self:mult(notContinuousRecord, naturalEndOfCycle, "EndOfCycle"))

  local continuousInc   = self:latch(punch.start, punch.last, "ContinuousInc")
  local continuousClock = self:mult(continuousInc, engagedClock, "ContinuousClock")
  connect(continuousClock, "Out", controls.continuousCount, "In")

  local resetContinuous = self:mult(continuousRecord, reset, "ResetContinuous")
  connect(resetContinuous, "Out", controls.continuousCount, "Reset")

  local easedFeedback = self:easeIn(punchSlew, controls.feedback, "EasedFeedback")
  local feedback = self:mult(notContinuousRecord, easedFeedback, "Feedback")

  local head = self:looper(channelCount, {
    feedback = feedback,
    engage   = engaged,
    reset    = reset,
    record   = punchSlew
  })

  local engagedRecordLevel = self:mult(punchSlew, engaged, "EngagedRecordLevel")
  local invRecordLevel     = self:logicalNot(engagedRecordLevel, "InvRecordLevel")
  local duckDry            = self:mult(controls.through, invRecordLevel, "DuckDry")

  self:output("Left", 1, {
    duckDry   = duckDry,
    wet       = head,
    wetOutlet = "Left Out"
  })

  if channelCount > 1 then
    self:output("Right", 2, {
      duckDry   = duckDry,
      wet       = head,
      wetOutlet = "Right Out"
    })
  end
end

function Sloop:easeIn(by, value, suffix)
  local name = function (str) return "Ease"..str..suffix end

  local nComplement = self:sum(self:mConst(-1), value, name("NegativeComplement"))
  local scaled      = self:mult(by, nComplement, name("Scaled"))

  return self:sum(self:mConst(1), scaled, "Feedback")
end

function Sloop:slew(input, fadeIn, fadeOut, suffix)
  local name = function (str) return "Slew"..str..suffix end

  local slewIn = self:createObject("SlewLimiter", name("In"))
  slewIn:setOptionValue("Direction", 1) -- Up
  tie(slewIn, "Time", fadeIn, "Out")
  connect(input, "Out", slewIn, "In")

  local slewOut = self:createObject("SlewLimiter", name("Out"))
  slewOut:setOptionValue("Direction", 3) -- Down
  tie(slewOut, "Time", fadeOut, "Out")
  connect(slewIn, "Out", slewOut, "In")

  return slewOut
end

function Sloop:looper(channelCount, args)
  local head = self:createObject("FeedbackLooper", "head", channelCount)
  connect(args.feedback, "Out", head, "Feedback")
  connect(args.engage,   "Out", head, "Engage")
  connect(args.reset,    "Out", head, "Reset")
  connect(args.record,   "Out", head, "Punch")

  local inputLeft = self:createObject("Multiply", "InputLeft")
  connect(self,        "In1", inputLeft, "Left")
  connect(args.record, "Out", inputLeft, "Right")
  connect(inputLeft,   "Out", head,      "Left In")

  if channelCount > 1 then
    local inputRight = self:createObject("Multiply", "InputRight")
    connect(self,        "In2", inputRight, "Left")
    connect(args.record, "Out", inputRight, "Right")
    connect(inputRight,  "Out", head,       "Right In")
  end

  return head
end

function Sloop:output(suffix, channel, args)
  local name = function (str) return str..suffix end

  local dry = self:createObject("Multiply", name("Dry"))
  connect(self,         "In"..channel, dry, "Left")
  connect(args.duckDry, "Out",         dry, "Right")

  local mix = self:createObject("Sum", name("Mix"))
  connect(dry,     "Out",           mix,  "Left")
  connect(args.wet, args.wetOutlet, mix,  "Right")
  connect(mix,     "Out",           self, "Out"..channel)
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

  local bufferMenuDescription = "Buffer"
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

  controls.recordingHeader = MenuHeader {
    description = "Recording"
  }
  menu[#menu + 1] = "recordingHeader"

  controls.recordMode = OptionControl {
    description      = "Mode",
    descriptionWidth = 2,
    option           = self._controls.continuousModeOption,
    choices          = { "continuous", "manual" },
    onUpdate         = function (choice)
      local length  = self._controls.length:getParameter("Bias")
      local counter = self._controls.continuousCount:getParameter("Value")

      local gate      = self._controls.continuousModeGate:getParameter("Value")
      local isGateOn  = gate:value() == 1
      local isGateOff = not isGateOn

      if choice == "continuous" and isGateOff then
        gate:hardSet(1)
        counter:hardSet(length:value())
        length:tie(counter)
      end

      if choice == "manual" and isGateOn then
        gate:hardSet(0)
        length:untie()
        length:hardSet(counter:value())
      end
    end
  }
  menu[#menu + 1] = "recordMode"

  controls.recordLength = OptionControl {
    description      = "Length",
    descriptionWidth = 2,
    option           = self._controls.fixedRecordOption,
    choices          = { "locked", "free" },
    onUpdate         = function (choice)
      local loopLength = self._controls.length:getParameter("Bias")
      local recordLength = self._controls.rLength:getParameter("Bias")

      if choice == "locked" then
        recordLength:tie(loopLength)
      else
        recordLength:hardSet(loopLength:value())
        recordLength:untie()
      end
    end
  }
  menu[#menu + 1] = "recordLength"

  controls.resetHeader = MenuHeader {
    description = "Force Reset On..."
  }
  menu[#menu + 1] = "resetHeader"

  controls.resetOnDisengage = OptionControl {
    description = "Disengage",
    descriptionWidth = 2,
    option      = self._controls.resetOnDisengageOption,
    choices     = { "yes", "no" },
    onUpdate    = function (choice)
      if choice == "yes" then
        self._controls.resetOnDisengageGate:hardSet("Value", 1)
      else
        self._controls.resetOnDisengageGate:hardSet("Value", 0)
      end
    end
  }
  menu[#menu + 1] = "resetOnDisengage"

  controls.resetOnRecord = OptionControl {
    description = "Record",
    descriptionWidth = 2,
    option      = self._controls.resetOnRecordOption,
    choices     = { "yes", "no" },
    onUpdate    = function (choice)
      if choice == "yes" then
        self._controls.resetOnRecordGate:hardSet("Value", 1)
      else
        self._controls.resetOnRecordGate:hardSet("Value", 0)
      end
    end
  }
  menu[#menu + 1] = "resetOnRecord"

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
    expanded  = { "clock", "record", "steps", "feedback", "through" },
    collapsed = { "wave2" },

    clock     = { "wave2", "clock", "engage", "reset" },
    record    = { "wave2", "record", "steps", "rSteps" },
    steps     = { "wave2", "record", "steps", "rSteps" },
    feedback  = { "wave2", "feedback", "fadeIn", "fadeOut" },
    through   = { "wave2", "record", "feedback", "through" }
  }

  local intMap = function (min, max)
    local map = app.LinearDialMap(min,max)
    map:setSteps(5, 1, 0.25, 0.25)
    map:setRounding(1)
    return map
  end

  controls.wave2 = RecordingView {
    head  = objects.head,
    width = 2 * ply
  }

  controls.wave3 = RecordingView {
    head  = objects.head,
    width = 3 * ply
  }

  controls.steps = GainBias {
    button        = "steps",
    description   = "Cycle Length",
    branch        = branches.length,
    gainbias      = objects.length,
    range         = objects.lengthRange,
    gainMap       = intMap(-config.maxSteps, config.maxSteps),
    biasMap       = intMap(1, config.maxSteps),
    biasPrecision = 0,
    initialBias   = config.initialSteps
  }

  controls.rSteps = GainBias {
    button        = "rSteps",
    description   = "Record Length",
    branch        = branches.rLength,
    gainbias      = objects.rLength,
    range         = objects.rLengthRange,
    gainMap       = intMap(-config.maxSteps, config.maxSteps),
    biasMap       = intMap(1, config.maxSteps),
    biasPrecision = 0,
    initialBias   = config.initialSteps
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
    initialBias   = config.initialThrough
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
    initialBias   = config.initialFeedback
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
    initialBias   = config.initialFadeIn
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
    initialBias   = config.initialFadeOut
  }

  self:showMenu(true)

  return controls, views
end

function Sloop:serialize()
  local t = Unit.serialize(self)
  if self.sample then
    t.sample         = SamplePool.serializeSample(self.sample)
    t.samplePosition = self.objects.head:getPosition()
  end

  t.resetOnDisengage = self._controls.resetOnRecordOption:value()
  t.resetOnRecord    = self._controls.resetOnRecordOption:value()
  t.enableFRecord    = self._controls.fixedRecordOption:value()
  t.continuousMode   = self._controls.continuousModeOption:value()

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

  self._controls.resetOnDisengageOption:set(t.resetOnDisengage)
  self._controls.resetOnRecordOption:set(t.resetOnRecord)
  self._controls.fixedRecordOption:set(t.enableFRecord)
  self._controls.continuousModeOption:set(t.continuousMode)
end

function Sloop:onRemove()
  self:setSample(nil)
  Unit.onRemove(self)
end

return Sloop
