-- luacheck: globals app connect
local app = app
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"
local Encoder = require "Encoder"
local SamplePool = require "Sample.Pool"
local OutputScope = require "Unit.ViewControl.OutputScope"
local SamplePoolInterface = require "Sample.Pool.Interface"
local OptionControl = require "Unit.MenuControl.OptionControl"
local Task = require "Unit.MenuControl.Task"
local MenuHeader = require "Unit.MenuControl.Header"
local Path = require "Path"
local ply = app.SECTION_PLY

local config = require "Strike.defaults"

local Strike = Class{}
Strike:include(Unit)

function Strike:init(args)
  args.title    = "Strike"
  args.mnemonic = "lpg"

  Unit.init(self, args)
end

function Strike:createControl(type, name)
  local control = self:createObject(type, name)
  local controlRange = self:createObject("MinMax", name.."Range")
  connect(control, "Out", controlRange, "In")
  self:createMonoBranch(name, control, "In", control, "Out")
  return control
end

function Strike:createTriggerControl(name)
  local gate = self:createObject("Comparator", name)
  gate:setMode(3)
  self:createMonoBranch(name, gate, "In", gate, "Out")
  return gate
end

function Strike:createToggleControl(name)
  local gate = self:createObject("Comparator", name)
  gate:setMode(1)
  self:createMonoBranch(name, gate, "In", gate, "Out")
  return gate
end

function Strike:createControls()
  self._controls = {
    strike = self:createTriggerControl("strike"),
    loop   = self:createToggleControl("loop"),
    lift   = self:createControl("GainBias", "lift"),
    cutoff = self:createControl("GainBias", "cutoff"),
    Q      = self:createControl("GainBias", "Q"),
    attack = self:createControl("GainBias", "attack"),
    aCurve = self:createControl("GainBias", "aCurve"),
    decay  = self:createControl("GainBias", "decay"),
    dCurve = self:createControl("GainBias", "dCurve"),
  }

  return self._controls
end

-- Create a memoized constant value to be used as an output.
function Strike:mConst(value)
  self.constants = self.constants or {}

  if self.constants[value] == nil then
    local const = self:createObject("Constant", "constant"..value)
    const:hardSet("Value", value)

    self.constants[value] = const
  end

  return self.constants[value]
end

function Strike:adapt(input, suffix)
  local adapter = self:createObject("ParameterAdapter", "adapt"..suffix)
  adapter:hardSet("Gain", 1)
  connect(input, "Out", adapter, "In")
  return adapter;
end

function Strike:cGate(input, suffix)
  local gate = self:createObject("Comparator", "ConstantGate"..suffix)
  connect(input, "Out", gate, "In")
  gate:setGateMode()
  return gate
end

function Strike:cTrig(input, suffix)
  local trig = self:createObject("Comparator", "ConstantTrig"..suffix)
  trig:hardSet("Hysteresis", 0)
  connect(input, "Out", trig, "In")
  trig:setTriggerMode()
  return trig
end

function Strike:toggle(name)
  local gate = self:createObject("Comparator", name)
  gate:hardSet("Hysteresis", 0)
  gate:setToggleMode()
  return gate
end

-- Add together two outputs.
function Strike:sum(left, right, suffix)
  local sum = self:createObject("Sum", "sum"..suffix)
  connect(left, "Out", sum, "Left")
  connect(right, "Out", sum, "Right")
  return sum
end

function Strike:clip(input, name)
  local clip = self:createObject("Clipper", name)
  connect(input, "Out", clip, "In")
  return clip
end

function Strike:logicalGateOr(left, right, suffix)
  local sum = self:sum(left, right, "Or"..suffix)
  return self:clip(sum, "LogicalOrClip"..suffix)
end

function Strike:logicalAnd(left, right, suffix)
  return self:mult(left, right, suffix)
end

function Strike:logicalOr(left, right, suffix)
  local negateRight = self:mult(self:mConst(-1), right, "Negate"..suffix)
  local difference = self:sum(left, negateRight, "Sum"..suffix)

  local leftHigh = self:cGate(difference, "LeftHigh"..suffix)
  local rightHigh = self:logicalNot(leftHigh, "RightHigh"..suffix)

  local leftPart = self:mult(leftHigh, left, "LeftPart"..suffix)
  local rightPart = self:mult(rightHigh, right, "RightPart"..suffix)

  local out = self:sum(leftPart, rightPart, "Or"..suffix)
  return out
end

function Strike:cGainBias(gain, bias, suffix)
  local gb = self:createObject("GainBias", "gainBias"..suffix)
  gb:hardSet("Gain", gain)
  gb:hardSet("Bias", bias)
  return gb
end

function Strike:logicalNot(input, suffix)
  local gb = self:cGainBias(-1, 1, "Not"..suffix)
  connect(input, "Out", gb, "In")
  return gb
end

function Strike:complement(input, suffix)
  return self:sum(self:mConst(1), self:nComplement(input, suffix), "c"..suffix)
end

function Strike:nComplement(input, suffix)
  return self:sum(self:mConst(-1), input, "nc"..suffix)
end

function Strike:mult(left, right, suffix)
  local mult = self:createObject("Multiply", "vca"..suffix)
  connect(left, "Out", mult, "Left")
  connect(right, "Out", mult, "Right")
  return mult
end

function Strike:rectify(input, type, suffix)
  local r = self:createObject("Rectify", "r"..suffix)
  r:setOptionValue("Type", type)
  connect(input, "Out", r, "In")
  return r;
end

function Strike:fullRectify(input, suffix)
  return self:rectify(input, 3, suffix)
end

function Strike:cBumpMap(center, width, height, fade, name)
  local bm = self:createObject("BumpMap", name)
  bm:hardSet("Center", center)
  bm:hardSet("Width", width)
  bm:hardSet("Height", height)
  bm:hardSet("Fade", fade)
  return bm
end

function Strike:sineEnv(trigger, duration, skew, suffix)
  local name = function (str) return str..suffix end

  local trig = self:cTrig(trigger, name("in"))

  local env = self:createObject("SkewedSineEnvelope", name("env"))
  -- env:hardSet("Level", 1)
  connect(trig, "Out", env, "Trigger")
  connect(self:mConst(1), "Out", env, "Level")

  local adaptSkew = self:adapt(self:mConst(skew), name("skew"))
  tie(env, "Skew", adaptSkew, "Out")

  local adaptDuration = self:adapt(duration, name("duration"))
  tie(env, "Duration", adaptDuration, "Out")

  return env
end

function Strike:directionCode(str)
  local lookup = {
    up   = 1,
    down = 3
  }

  return lookup[str]
end

function Strike:cSlew(time, direction, name)
  local slew = self:createObject("SlewLimiter", name)
  slew:setOptionValue("Direction", self:directionCode(direction))

  local adaptTime = self:adapt(time, name("time"))
  tie(slew, "Time", adaptTime, "Out")

  return slew
end

function Strike:slew(input, time, direction, suffix)
  local name = function (str) return str..suffix end

  local slew = self:createObject("SlewLimiter", name("slew"))
  slew:setOptionValue("Direction", self:directionCode(direction))
  connect(input, "Out", slew, "In")

  local adaptTime = self:adapt(time, name("time"))
  tie(slew, "Time", adaptTime, "Out")

  return slew;
end

function Strike:latch(input, reset, suffix)
  local name = function (str) return "Latch"..str..suffix end

  local high = self:createObject("Counter", name("High"))
  high:hardSet("Start", 0)
  high:hardSet("Finish", 1)
  high:hardSet("Step Size", 1)
  high:setOptionValue("Processing Rate", 2) -- sample rate
  high:setOptionValue("Wrap", 0)

  connect(input, "Out", high, "In")
  connect(reset, "Out", high, "Reset")

  return high
end

function Strike:loadSample()
  if self.sample then
    return self.sample
  end

  local lib      = self.loadInfo.libraryName
  local filename = config.expFile
  local path     = Path.join("1:/ER-301/libs", lib, filename)
  local sample   = SamplePool.load(path)

  if not sample then
    local Overlay = require "Overlay"
    Overlay.mainFlashMessage("Could not load %s.", path)
  end

  self.sample = sample
  return sample
end

function Strike:sampleMap(name)
  local bumpMap = self:createObject("BumpMap", name)
  bumpMap:setSample(self:loadSample().pSample)

  bumpMap:hardSet("Center", 0)
  bumpMap:hardSet("Width", 2.0)
  bumpMap:hardSet("Height", 1.0)
  bumpMap:hardSet("Fade", 0)
  bumpMap:setOptionValue("Interpolation", 1) -- none

  return bumpMap
end

function Strike:pick(gate, left, right, suffix)
  local name = function (str) return "Pick"..str..suffix end

  local notGate   = self:logicalNot(gate, name("NotGate"))
  local pickLeft  = self:mult(left, gate, name("PickLeft"))
  local pickRight = self:mult(right, notGate, name("PickRight"))

  return self:sum(pickLeft, pickRight, name("Out"))
end

-- Mix center and side based on the input;
--    0     gives center signal
--   -1, 1  gives side signal
function Strike:mix(input, center, side, suffix)
  local name = function (str) return "Mix"..str..suffix end

  local sideAmount   = self:fullRectify(input, name("SideAmount"))
  local centerAmount = self:logicalNot(sideAmount, name("CenterAmount"))

  local sidePart   = self:mult(side, sideAmount, name("SidePart"))
  local centerPart = self:mult(center, centerAmount, name("CenterPart"))

  return self:sum(sidePart, centerPart, name("Out"))
end

function Strike:curve(to, direction, time, curve, suffix)
  local name = function (str) return "SlewCurve"..str..suffix end

  local slew = self:slew(to, time, direction, name("Slew"))

  -- Exponential: read the sample from 0 to -1
  -- Logarithmic: Read the sample from 0 to 1
  local expHead = self:sum(self:mConst(-1), slew, name("ExpHead"))
  local logHead = self:logicalNot(slew, name("LogHead"))

  local isExponential = self:cGate(curve, name("IsExponential"))
  local head          = self:pick(isExponential, expHead, logHead, name("Head"))

  return slew, head
end

function Strike:envelope(gate, loop, attack, aCurve, decay, dCurve, suffix)
  local name = function (str) return str..suffix end

  local trigger = self:cTrig(gate, name("Trigger"))

  local eor = self:createObject("Comparator", name("EOR"))
  eor:hardSet("Threshold", 0.995)
  eor:hardSet("Hysteresis", 0)
  eor:setTriggerMode()

  local eof = self:createObject("Comparator", name("EOF"))
  eof:hardSet("Threshold", 0.995)
  eof:hardSet("Hysteresis", 0)
  eof:setTriggerMode()



  local riseLatch = self:latch(trigger, eor, name("RiseLatch"))
  local rising    = self:logicalGateOr(gate, riseLatch, name("Rising"))
  local notRising = self:logicalNot(rising, name("NotRising"))

  local fall, fallHead = self:curve(rising, "down", decay, dCurve, name("Fall"))
  local fallMask = self:mult(fall, notRising, name("FallMask"))

  local riseIn = self:logicalOr(fallMask, rising, name("RiseIn"))
  local rise, riseHead = self:curve(riseIn, "up", attack, aCurve, name("Rise"))
  connect(rise, "Out", eor, "In")

  local riseMask = self:mult(rise, rising, name("RiseMask"))

  local head = self:pick(rising, riseHead, fallHead, name("Head"))
  local map = self:sampleMap(name("Map"))
  connect(head, "Out", map, "In")

  local slew = self:logicalOr(riseMask, fallMask, name("Slew"))
  local amount = self:pick(rising, aCurve, dCurve, name("Amount"))
  local out = self:mix(amount, slew, map, name("Out"))

  return out
end

function Strike:onLoadGraph(channelCount)
  local controls = self:createControls()

  local envelope = self:envelope(
    controls.strike,
    controls.loop,
    controls.attack,
    controls.aCurve,
    controls.decay,
    controls.dCurve,
    "ar"
  )


  local lift = self:mult(controls.lift, controls.lift, "liftLift")
  local levelLift = envelope
  local freqLift  = self:mult(envelope, lift, "adsrLift")
  local cutoff = self:mult(freqLift, controls.cutoff, "cutoffLift")

  local filter = self:createObject("StereoLadderFilter", "filter")
  -- filter:hardSet("Fundamental", 100000)

  connect(self,     "In1", filter, "Left In")
  -- connect(freqLift, "Out", filter, "V/Oct")
  connect(controls.Q, "Out", filter, "Resonance")
  connect(cutoff, "Out", filter, "Fundamental")

  local vcaLeft = self:createObject("Multiply", "vcaLeft")
  connect(levelLift, "Out",      vcaLeft, "Left")
  connect(filter,    "Left Out", vcaLeft, "Right")

  connect(envelope, "Out", self, "Out1")
  -- connect(filter, "Left Out", self, "Out1")

  if channelCount > 1 then
    connect(self, "In2", filter, "Right In")

    local vcaRight = self:createObject("Multiply", "vcaRight")
    connect(levelLift, "Out",       vcaRight, "Left")
    connect(filter,    "Right Out", vcaRight, "Right")

    connect(vcaRight, "Out", self, "Out2")
  end
end

function Strike:onLoadViews(objects, branches)
  local controls, views = {}, {
    expanded  = { "strike", "loop", "lift", "attack", "decay" },
    collapsed = { "strike", "loop" },

    strike    = { "wave3", "strike", "loop" },
    loop      = { "wave3", "strike", "loop" },
    lift      = { "wave2", "lift", "cutoff", "Q" },
    attack    = { "wave3", "attack", "aCurve" },
    decay     = { "wave3", "decay", "dCurve" }
  }

  local intMap = function (min, max)
    local map = app.LinearDialMap(min, max)
    map:setSteps(5, 0.1, 0.25, 0.25)
    map:setRounding(0.1)
    return map
  end

  local fineMap = function (min, max)
    local map = app.LinearDialMap(min, max)
    map:setSteps(0.1, 0.01, 0.001, 0.001)
    map:setRounding(0.001)
    return map
  end

  controls.wave1 = OutputScope {
    monitor = self,
    width   = 1 * ply
  }

  controls.wave2 = OutputScope {
    monitor = self,
    width   = 2 * ply
  }

  controls.wave3 = OutputScope {
    monitor = self,
    width   = 3 * ply
  }

  controls.strike = Gate {
    button      = "strike",
    description = "Hit Me!",
    branch      = branches.strike,
    comparator  = objects.strike
  }

  controls.loop = Gate {
    button      = "loop",
    description = "Loop",
    branch      = branches.loop,
    comparator  = objects.loop
  }

  controls.lift = GainBias {
    button        = "lift",
    description   = "How Bright?",
    branch        = branches.lift,
    gainbias      = objects.lift,
    range         = objects.liftRange,
    biasMap       = fineMap(0, 1),
    biasUnits     = app.unitNone,
    biasPrecision = 3,
    initialBias   = config.initialLift
  }

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

  controls.Q = GainBias {
    button        = "Q",
    description   = "Resonance",
    branch        = branches.Q,
    gainbias      = objects.Q,
    range         = objects.QRange,
    biasMap       = fineMap(0, 1),
    biasUnits     = app.unitNone,
    biasPrecision = 3,
    initialBias   = config.initialQ
  }

  controls.attack = GainBias {
    button        = "attack",
    description   = "Attack Time",
    branch        = branches.attack,
    gainbias      = objects.attack,
    range         = objects.attackRange,
    biasMap       = fineMap(0.01, 10),
    biasUnits     = app.unitSecs,
    biasPrecision = 3,
    initialBias   = config.initialAttack
  }

  controls.aCurve = GainBias {
    button        = "aCurve",
    description   = "Attack Curve",
    branch        = branches.aCurve,
    gainbias      = objects.aCurve,
    range         = objects.aCurveRange,
    biasMap       = fineMap(-1, 1),
    biasPrecision = 3,
    initialBias   = 0
  }

  controls.decay = GainBias {
    button        = "decay",
    description   = "Decay Time",
    branch        = branches.decay,
    gainbias      = objects.decay,
    range         = objects.decayRange,
    biasMap       = fineMap(0.01, 10),
    biasUnits     = app.unitSecs,
    biasPrecision = 3,
    initialBias   = config.initialDecay
  }

  controls.dCurve = GainBias {
    button        = "dCurve",
    description   = "Attack Curve",
    branch        = branches.dCurve,
    gainbias      = objects.dCurve,
    range         = objects.dCurveRange,
    biasMap       = fineMap(-1, 1),
    biasPrecision = 3,
    initialBias   = 0
  }

  return controls, views
end

return Strike