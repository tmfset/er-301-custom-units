-- luacheck: globals app connect
local app = app
local core = require "core.libcore"
local lib = require "sloop.libSloop"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"
local Encoder = require "Encoder"
local SamplePool = require "Sample.Pool"
local RecordingView = require "core.Looper.RecordingView"
local SamplePoolInterface = require "Sample.Pool.Interface"
local OptionControl = require "Unit.MenuControl.OptionControl"
local Task = require "Unit.MenuControl.Task"
local MenuHeader = require "Unit.MenuControl.Header"
local pool = require "Sample.Pool"
local ply = app.SECTION_PLY

local config = require "Sloop.defaults"

local Sloop = Class {}
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
    local const = self:addObject("constant"..value, app.Constant())
    const:hardSet("Value", value)

    self.constants[value] = const
  end

  return self.constants[value]
end

function Sloop:addToMix(mix, index, input)
  connect(input, "Out", mix, "In"..index)
  -- mix:hardSet("Gain"..index, 1)
end

-- Add together two outputs.
function Sloop:sum(left, right, suffix)
  local sum = self:addObject("sum"..suffix, app.Sum())
  connect(left, "Out", sum, "Left")
  connect(right, "Out", sum, "Right")
  return sum
end

-- Multiply two outputs.
function Sloop:mult(left, right, suffix)
  local mult = self:addObject("vca"..suffix, app.Multiply())
  connect(left, "Out", mult, "Left")
  connect(right, "Out", mult, "Right")
  return mult
end

function Sloop:clip(input, name)
  local clip = self:addObject(name, core.Clipper())
  connect(input, "Out", clip, "In")
  return clip
end

function Sloop:logicalAnd(left, right, suffix)
  -- return self:mult(left, right, suffix)
  local op = self:addObject("And"..suffix, lib.And())
  connect(left, "Out", op, "Left")
  connect(right, "Out", op, "Right")
  return op
end

function Sloop:logicalOr(left, right, suffix)
  -- local sum = self:sum(left, right, "Or"..suffix)
  -- return self:clip(sum, "LogicalOrClip"..suffix)
  local op = self:addObject("Or"..suffix, lib.Or(2))
  connect(left, "Out", op, "In1")
  connect(right, "Out", op, "In2")
  return op
end

function Sloop:logicalNot(input, suffix)
  -- local gb = self:cGainBias(-1, 1, "Not"..suffix)
  -- connect(input, "Out", gb, "In")
  -- return gb
  local op = self:addObject("Not"..suffix, lib.Not())
  connect(input, "Out", op, "In")
  return op
end

-- Constant gain bias.
function Sloop:cGainBias(gain, bias, suffix)
  local gb = self:addObject("gainBias"..suffix, app.GainBias())
  gb:hardSet("Gain", gain)
  gb:hardSet("Bias", bias)
  return gb
end

function Sloop:cGate(input, suffix)
  local gate = self:addObject("ConstantGate"..suffix, app.Comparator())
  connect(input, "Out", gate, "In")
  gate:setGateMode()
  return gate
end

function Sloop:cTrig(input, suffix)
  local op = self:addObject("Trig"..suffix, lib.Trig())
  connect(input, "Out", op, "In")
  return op
  -- local trig = self:addObject("ConstantTrig"..suffix, app.Comparator())
  -- connect(input, "Out", trig, "In")
  -- return trig
end

function Sloop:toggle(name)
  local gate = self:addObject(name, app.Comparator())
  gate:setToggleMode()
  return gate
end

-- A simple counter.
function Sloop:newCounter(start, finish, stepSize, gain, name, default)
  local counter = self:addObject(name, core.Counter())
  counter:setOptionValue("Processing Rate", 2) -- Sample Rate
  counter:hardSet("Start", start)
  counter:hardSet("Finish", finish)
  counter:hardSet("Step Size", stepSize)
  counter:hardSet("Gain", gain)
  counter:hardSet("Value", default)
  return counter
end

function Sloop:latch(input, reset, suffix)
  -- local name = function (str) return "Latch"..str..suffix end

  -- local high = self:addObject(name("High"), core.Counter())
  -- high:hardSet("Start", 0)
  -- high:hardSet("Finish", 1)
  -- high:hardSet("Step Size", 1)
  -- high:hardSet("Gain", 1)
  -- high:hardSet("Bias", 0)
  -- high:setOptionValue("Processing Rate", 2) -- sample rate
  -- high:setOptionValue("Wrap", 2) -- no

  -- connect(input, "Out", high, "In")
  -- connect(reset, "Out", high, "Reset")

  -- return high
  local name = function (str) return "Latch"..str..suffix end

  local high = self:addObject(name("High"), lib.Latch())
  connect(input, "Out", high, "Set")
  connect(reset, "Out", high, "Reset")

  return high
end

-- A latch that outputs the next clock tick after being set by the input.
function Sloop:clockResetLatch(input, clock, suffix)
  local name = function (str) return "ClockLatch"..str..suffix end

  local reset = self:addObject(name("Reset"), app.Multiply())
  local latch = self:latch(input, reset, name("Latch"))
  connect(latch, "Out", reset, "Left")
  connect(clock, "Out", reset, "Right")

  return reset
end

-- A clocked gate. When `gate` is high, output goes high on the next clock
-- tick. When `gate` is low, output goes low on the next clock tick.
function Sloop:clockGate(clock, gate, reset, suffix)
  local name = function (str) return "DLatch"..str..suffix end

  local latch = self:addObject(name("Latch"), lib.DLatch())
  connect(gate, "Out", latch, "In")
  connect(clock, "Out", latch, "Clock")
  connect(reset, "Out", latch, "Reset")

  return latch
  -- local name = function (str) return "ClockGate"..str..suffix end

  -- local input      = self:logicalAnd(clock, gate, name("Input"))
  -- local invGate    = self:logicalNot(gate, name("InvGate"))
  -- local clockReset = self:logicalAnd(clock, invGate, name("clockReset"))
  -- local reset      = self:logicalOr(clockReset, forceReset, name("Reset"))

  -- return self:latch(input, reset, name("Latch"))
end

function Sloop:adapt(input, gain, bias, suffix)
  local adapter = self:addObject("adapt"..suffix, app.ParameterAdapter())
  adapter:hardSet("Gain", gain)
  adapter:hardSet("Bias", bias)
  connect(input, "Out", adapter, "In")
  return adapter;
end

function Sloop:countDownGate(clock, length, reset, suffix)
  local name = function (str) return "CountDownGate"..str..suffix end

  local counter = self:addObject(name("Count"), core.Counter())
  counter:setOptionValue("Processing Rate", 2) -- Sample Rate
  counter:hardSet("Start", 0)
  counter:hardSet("Step Size", -1)
  counter:hardSet("Gain", 0.25)

  local finish = self:adapt(length, 1, -1, name("Finish"))
  tie(counter, "Finish", finish, "Out")

  local wait    = self:cGate(counter, name("Gate"))
  local stop    = self:logicalNot(wait, name("NotGate"))
  local input   = self:logicalAnd(wait, clock, name("Input"))

  connect(input, "Out", counter, "In")
  connect(reset, "Out", counter, "Reset")

  return stop
end

function Sloop:countDown(clock, length, reset, suffix)
  local name = function (str) return "CountDown"..str..suffix end

  local gate = self:countDownGate(clock, length, reset, name("Gate"))
  return self:logicalAnd(clock, gate, name("Output"))
end

function Sloop:clockCountGate(clock, length, gate, bypass, suffix)
  local name = function (str) return "ClockCountGate"..str..suffix end

  local notGate = self:logicalNot(gate, name("NotGate"))
  local startSignal = self:clockResetLatch(gate, clock, name("StartSignal"))

  local countDownEnd  = self:countDownGate(clock, length, startSignal, name("CountDownEnd"))
  local counting = self:latch(startSignal, countDownEnd, name("Counting"))
  local notCounting = self:logicalNot(counting, name("NotCounting"))
  local notCountingBypass = self:logicalOr(notCounting, bypass, name("NotCountingBypass"))

  local disabled = self:logicalAnd(notCountingBypass, notGate, name("Disabled"))
  local stopSignal = self:logicalAnd(disabled, clock, name("StopSignal"))

  local output = self:latch(startSignal, stopSignal, name("Output"))

  return {
    start = startSignal,
    gate  = output,
    last  = self:logicalAnd(output, disabled, name("Last")),
    stop  = self:logicalAnd(output, stopSignal, name("ActualStop"))
  }
end

function Sloop:createControl(name, type)
  local control = self:addObject(name, type)
  local controlRange = self:addObject(name.."Range", app.MinMax())
  connect(control, "Out", controlRange, "In")
  self:addMonoBranch(name, control, "In", control, "Out")
  return control
end

function Sloop:createGainBiasControl(name, default)
  local control = self:createControl(name, app.GainBias())
  control:hardSet("Bias", default)
  return control
end

function Sloop:setInitialBuffer(channelCount)
  if self.sample then
    return
  end

  local length = config.defaultBufferLength
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

function Sloop:createComparatorControl(name, mode, default)
  local gate = self:addObject(name, app.Comparator())
  gate:setMode(mode)
  self:addMonoBranch(name, gate, "In", gate, "Out")
  if default then
    gate:setOptionValue("State", default)
  end
  return gate
end

function Sloop:createToggleControl(name, default)
  return self:createComparatorControl(name, 1, default)
end

function Sloop:createGateControl(name)
  return self:createComparatorControl(name, 2)
end

function Sloop:createTriggerControl(name)
  return self:createComparatorControl(name, 3)
end

function Sloop:createConstantControl(name)
  return self:addObject(name, app.Constant())
end

local function pTie(left, right)
  local untie = function (from, to)
    if from:isTied() then
      from:untie()
      from:hardSet(to:value())
    end
  end

  return {
    bindLeft    = function () left:tie(right) end,
    unbindLeft  = function () untie(left, right) end,
    bindRight   = function () right:tie(left) end,
    unbindRight = function () untie(right, left) end
  }
end

local function syncTieLeft(tie)
  return function (option)
    if option:value() == 1 then
      tie.bindLeft()
    else
      tie.unbindLeft()
    end
  end
end

local function syncTieSwap(tie)
  return function (option)
    if option:value() == 1 then
      tie.unbindRight()
      tie.bindLeft()
    else
      tie.unbindLeft()
      tie.bindRight()
    end
  end
end

local function syncConstant(constant)
  return function (option)
    constant:hardSet("Value", 2 - option:value())
  end
end

function Sloop:createOption(name, syncs, default)
  local option = app.Option(name)

  local sync = function ()
    for _, sync in pairs(syncs) do
      sync(option)
    end
  end

  local set = function (value)
    -- if value ~= nil then
    --   option:set(value)
    -- elseif default ~= nil then
    --   option:set(default)
    -- end
    sync()
  end

  local control = {
    option = option,
    value  = function () return option:value() end,
    set    = set,
    sync   = sync
  }

  control.set(default)
  return control
end

function Sloop:createControls()
  self._controls = {
    length           = self:createGainBiasControl("length", config.initialSteps),
    rLength          = self:createGainBiasControl("rLength", config.initialSteps),

    clock            = self:createTriggerControl("clock"),
    engage           = self:createToggleControl("engage", config.startEngaged),
    record           = self:createGateControl("record"),
    reset            = self:createTriggerControl("reset"),
    through          = self:createControl("through", app.GainBias()),
    feedback         = self:createControl("feedback", app.GainBias()),
    fadeIn           = self:createControl("fadeIn", app.ParameterAdapter()),
    fadeOut          = self:createControl("fadeOut", app.ParameterAdapter()),

    resetOnDisengage = self:createConstantControl("ResetOnDisengageGate"),
    resetOnRecord    = self:createConstantControl("ResetOnRecordGate"),
    continuousMode   = self:createConstantControl("ContinuousModeGate"),

    continuousCount  = self:newCounter(1, config.maxSteps, 1, 1 / config.maxSteps, "ContinuousCount", config.initialSteps)
  }

  self._options = {
    resetOnDisengage = self:createOption("ResetOnDisengageOption", {
      syncConstant(self._controls.resetOnDisengage)
    }, config.defaultResetOnDisengage),

    resetOnRecord = self:createOption("ResetOnRecordOption", {
      syncConstant(self._controls.resetOnRecord)
    }, config.defaultResetOnRecord),

    continuousMode = self:createOption("ContinuousModeOption", {
      syncConstant(self._controls.continuousMode),
      syncTieSwap(pTie(
        self._controls.length:getParameter("Bias"),
        self._controls.continuousCount:getParameter("Value")
      ))
    }, config.defaultMode),

    fixedRecord = self:createOption("FixedRecordOption", {
      syncTieLeft(pTie(
        self._controls.rLength:getParameter("Bias"),
        self._controls.length:getParameter("Bias")
      ))
    }, config.defaultLengthMode)
  }

  return self._controls
end

function Sloop:onLoadGraph(channelCount)
  local controls = self:createControls()

  local clock = self:cTrig(controls.clock, "ClockTrigger")

  local disengaged       = self:logicalNot(controls.engage, "Disengaged")
  local resetOnDisengage = self:logicalAnd(disengaged, controls.resetOnDisengage, "ResetOnDisengageSignal")

  local engaged      = self:clockGate(clock, controls.engage, resetOnDisengage, "Engaged")
  local engagedClock = self:logicalAnd(clock, controls.engage, "EngagedClock")

  local punch     = self:clockCountGate(engagedClock, controls.rLength, controls.record, controls.continuousMode, "Punch")
  local punchSlew = self:slew(punch.gate, controls.fadeIn, controls.fadeOut, "Punch")

  local resetOnRecord    = self:logicalOr(controls.resetOnRecord, controls.continuousMode, "ResetOnRecord")
  local resetByForceTrig = self:clockResetLatch(controls.reset, engagedClock, "ManualResetLatch")

  local reset = self:addObject("Reset", lib.Or(5))
  self:addToMix(reset, 1, self:logicalAnd(resetOnRecord, punch.start, "ResetonRecordSignal"))
  self:addToMix(reset, 2, resetByForceTrig)
  self:addToMix(reset, 3, resetOnDisengage)
  self:addToMix(reset, 4, self:logicalAnd(punch.stop, controls.continuousMode, "ContinuousReset"))

  local continuousRecord    = self:logicalAnd(punch.gate, controls.continuousMode, "ContinuousRecord")
  local notContinuousRecord = self:logicalNot(continuousRecord, "NotContinuousRecord")
  local naturalEndOfCycle   = self:countDown(engagedClock, controls.length, reset, "NaturalEndOfCycle")
  self:addToMix(reset, 5, self:logicalAnd(notContinuousRecord, naturalEndOfCycle, "EndOfCycle"))

  local continuousInc   = self:latch(punch.start, punch.last, "ContinuousInc")
  local continuousClock = self:logicalAnd(continuousInc, engagedClock, "ContinuousClock")
  connect(continuousClock, "Out", controls.continuousCount, "In")

  local resetContinuous = self:logicalAnd(continuousRecord, reset, "ResetContinuous")
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

  self:setInitialBuffer(channelCount)
end

function Sloop:easeIn(by, value, suffix)
  local name = function (str) return "Ease"..str..suffix end

  local nComplement = self:sum(self:mConst(-1), value, name("NegativeComplement"))
  local scaled      = self:mult(by, nComplement, name("Scaled"))

  return self:sum(self:mConst(1), scaled, "Feedback")
end

function Sloop:slew(input, fadeIn, fadeOut, suffix)
  local name = function (str) return "Slew"..str..suffix end

  local slewIn = self:addObject(name("In"), core.SlewLimiter())
  slewIn:setOptionValue("Direction", 1) -- Up
  tie(slewIn, "Time", fadeIn, "Out")
  connect(input, "Out", slewIn, "In")

  local slewOut = self:addObject(name("Out"), core.SlewLimiter())
  slewOut:setOptionValue("Direction", 3) -- Down
  tie(slewOut, "Time", fadeOut, "Out")
  connect(slewIn, "Out", slewOut, "In")

  return slewOut
end

function Sloop:looper(channelCount, args)
  local head = self:addObject("head", core.FeedbackLooper(channelCount))
  connect(args.feedback, "Out", head, "Feedback")
  connect(args.engage,   "Out", head, "Engage")
  connect(args.reset,    "Out", head, "Reset")
  connect(args.record,   "Out", head, "Punch")

  local inputLeft = self:addObject("InputLeft", app.Multiply())
  connect(self,        "In1", inputLeft, "Left")
  connect(args.record, "Out", inputLeft, "Right")
  connect(inputLeft,   "Out", head,      "Left In")

  if channelCount > 1 then
    local inputRight = self:addObject("InputRight", app.Multiply())
    connect(self,        "In2", inputRight, "Left")
    connect(args.record, "Out", inputRight, "Right")
    connect(inputRight,  "Out", head,       "Right In")
  end

  return head
end

function Sloop:output(suffix, channel, args)
  local name = function (str) return str..suffix end

  local dry = self:addObject(name("Dry"), app.Multiply())
  connect(self,         "In"..channel, dry, "Left")
  connect(args.duckDry, "Out",         dry, "Right")

  local mix = self:addObject(name("Mix"), app.Sum())
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

  controls.recordMode = OptionControl {
    description      = "Mode",
    descriptionWidth = 2,
    option           = self._options.continuousMode.option,
    choices          = { "continuous", "manual" },
    onUpdate         = self._options.continuousMode.sync
  }
  menu[#menu + 1] = "recordMode"

  controls.recordLength = OptionControl {
    description      = "Length",
    descriptionWidth = 2,
    option           = self._options.fixedRecord.option,
    choices          = { "locked", "free" },
    onUpdate         = self._options.fixedRecord.sync
  }
  menu[#menu + 1] = "recordLength"

  controls.resetHeader = MenuHeader {
    description = "Reset On..."
  }
  menu[#menu + 1] = "resetHeader"

  controls.resetOnDisengage = OptionControl {
    description      = "Disengage",
    descriptionWidth = 2,
    option           = self._options.resetOnDisengage.option,
    choices          = { "yes", "no" },
    onUpdate         = self._options.resetOnDisengage.sync
  }
  menu[#menu + 1] = "resetOnDisengage"

  controls.resetOnRecord = OptionControl {
    description      = "Record",
    descriptionWidth = 2,
    option           = self._options.resetOnRecord.option,
    choices          = { "yes", "no" },
    onUpdate         = self._options.resetOnRecord.sync
  }
  menu[#menu + 1] = "resetOnRecord"


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
    biasPrecision = 2,
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
    biasPrecision = 2,
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

  return controls, views
end

function Sloop:serialize()
  local t = Unit.serialize(self)
  if self.sample then
    t.sample         = SamplePool.serializeSample(self.sample)
    t.samplePosition = self.objects.head:getPosition()
  end

  t.resetOnDisengage = self._options.resetOnDisengage.value()
  t.resetOnRecord    = self._options.resetOnRecord.value()
  t.continuousMode   = self._options.continuousMode.value()
  t.enableFRecord    = self._options.fixedRecord.value()

  return t
end

function Sloop:deserialize(t)
  -- -- We are not guaranteed to see our serialized values since a unit can be
  -- -- replaced wholesale. Luckily the set function accounts for nil values.
  -- self._options.resetOnDisengage.set(t.resetOnDisengage)
  -- self._options.resetOnRecord.set(t.resetOnRecord)
  -- self._options.continuousMode.set(t.continuousMode)
  -- self._options.fixedRecord.set(t.enableFRecord)

  -- -- Deserialize the remaining values *after* setting the options to avoid
  -- -- syncing issues due to parameter ties.
  -- Unit.deserialize(self, t)

  -- if t.sample then
  --   local sample = SamplePool.deserializeSample(t.sample)
  --   if sample then
  --     self:setSample(sample)
  --     if t.samplePosition then
  --       self.objects.head:setPosition(t.samplePosition)
  --     end
  --   else
  --     local Utils = require "Utils"
  --     app.log("%s:deserialize: failed to load sample.",self)
  --     Utils.pp(t.sample)
  --   end
  -- end
end

function Sloop:onRemove()
  self:setSample(nil)
  Unit.onRemove(self)
end

return Sloop
