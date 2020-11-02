-- luacheck: globals app connect
local app = app
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"
local Encoder = require "Encoder"
local OutputScope = require "Unit.ViewControl.OutputScope"
local ply = app.SECTION_PLY

local config = require "Strike.defaults"

local Strike = Class {}
Strike:include(Unit)

function Strike:init(args)
  args.title    = "LPG"
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
    strike   = self:createTriggerControl("strike"),
    loop     = self:createToggleControl("loop"),

    lift     = self:createControl("GainBias", "lift"),
    peak     = self:createControl("GainBias", "peak"),
    Q        = self:createControl("GainBias", "Q"),

    time     = self:createControl("GainBias", "time"),
    attack   = self:createControl("GainBias", "attack"),
    decay    = self:createControl("GainBias", "decay"),

    curve    = self:createControl("GainBias", "curve"),
    curveIn  = self:createControl("GainBias", "curveIn"),
    curveOut = self:createControl("GainBias", "curveOut"),
  }

  return self._controls
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

function Strike:mult(left, right, suffix)
  local mult = self:createObject("Multiply", "vca"..suffix)
  connect(left, "Out", mult, "Left")
  connect(right, "Out", mult, "Right")
  return mult
end

function Strike:slew(input, time, direction, suffix)
  local slew = self:createObject("SlewLimiter", "Slew"..suffix)
  slew:setOptionValue("Direction", direction)
  connect(input, "Out", slew, "In")
  tie(slew, "Time", time, "Out")
  return slew;
end

function Strike:follow(input, attack, decay, suffix)
  local follow = self:createObject("EnvelopeFollower", "Follow"..suffix)
  connect(input, "Out", follow, "In")
  tie(follow, "Attack Time", attack, "Out")
  tie(follow, "Release Time", decay, "Out")
  return follow
end

function Strike:latch(input, reset, suffix)
  local name = function (str) return "Latch"..str..suffix end

  local high = self:createObject("Counter", name("High"))
  high:hardSet("Start", 0)
  high:hardSet("Finish", 1)
  high:hardSet("Step Size", 1)
  high:setOptionValue("Processing Rate", 2) -- sample rate
  high:setOptionValue("Wrap", 2)

  connect(input, "Out", high, "In")
  connect(reset, "Out", high, "Reset")

  return high
end

function Strike:pick(gate, notGate, left, right, suffix)
  local name = function (str) return "PrePick"..str..suffix end

  local pickLeft  = self:mult(left, gate, name("PickLeft"))
  local pickRight = self:mult(right, notGate, name("PickRight"))

  return self:sum(pickLeft, pickRight, name("Out"))
end

-- Mix center and side based on the input;
--    0 gives center signal
--    1 gives side signal
function Strike:mix(input, center, side, suffix)
  local name = function (str) return "Mix"..str..suffix end

  local sideAmount   = input
  local centerAmount = self:logicalNot(sideAmount, name("CenterAmount"))

  local sidePart   = self:mult(side, sideAmount, name("SidePart"))
  local centerPart = self:mult(center, centerAmount, name("CenterPart"))

  return self:sum(sidePart, centerPart, name("Out"))
end

function Strike:envelope(args, suffix)
  local name = function (str) return "Envelope"..str..suffix end

  local scaledAttack = self:mult(args.time, args.attack, name("ScaledAttack"))
  local scaledDecay  = self:mult(args.time, args.decay, name("Decay"))

  local adaptAttack  = self:adapt(scaledAttack, name("AdaptAttack"))
  local adaptDecay   = self:adapt(scaledDecay, name("AdaptDecay"))

  local loopTrigger = self:createObject("Multiply", name("LoopTrigger"))
  connect(args.loop, "Out", loopTrigger, "Left")

  local eor = self:createObject("Comparator", name("EOR"))
  eor:hardSet("Threshold", 0.995)
  eor:hardSet("Hysteresis", 0)
  eor:setGateMode()

  local inputTrigger = self:cTrig(args.gate, name("InputTrigger"))
  local riseTrigger = self:logicalGateOr(inputTrigger, loopTrigger, name("RiseTrigger"))

  local riseLatch   = self:latch(riseTrigger, eor, name("RiseLatch"))
  local isRising    = self:logicalGateOr(riseLatch, args.gate, name("IsRising"))
  local isNotRising = self:logicalNot(isRising, name("IsNotRising"))

  local fall = self:slew(isRising, adaptDecay, 3, name("Fall"))
  local rise = self:slew(fall, adaptAttack, 1, name("Rise"))
  connect(rise, "Out", eor, "In")

  local curve = self:follow(isRising, adaptAttack, adaptDecay, name("Curve"))

  local curveAmount       = self:pick(isRising, isNotRising, args.curveIn, args.curveOut, name("CurveAmount"))
  local scaledCurveAmount = self:mult(curveAmount, args.curve, name("ScaledCurveAmount"))
  local out               = self:mix(scaledCurveAmount, rise, curve, name("Out"))

  local outGate = self:createObject("Comparator", name("OutGate"))
  outGate:hardSet("Threshold", 0.005)
  outGate:hardSet("Hysteresis", 0)
  outGate:setGateMode()
  connect(rise, "Out", outGate, "In")

  local eof = self:logicalNot(outGate, name("EOF"))
  connect(eof, "Out", loopTrigger, "Right")

  return out
end

function Strike:onLoadGraph(channelCount)
  local controls = self:createControls()

  local envelope = self:envelope({
    gate     = controls.strike,
    loop     = controls.loop,
    time     = controls.time,
    attack   = controls.attack,
    decay    = controls.decay,
    curve    = controls.curve,
    curveIn  = controls.curveIn,
    curveOut = controls.curveOut
  }, "Envelope")

  local lift = self:mult(envelope, controls.lift, "Lift")
  local peak = self:mult(lift, controls.peak, "Peak")
  local Q    = controls.Q

  local filter = self:createObject("StereoLadderFilter", "filter")
  connect(Q,    "Out", filter, "Resonance")
  connect(peak, "Out", filter, "Fundamental")
  connect(self, "In1", filter, "Left In")

  local vcaLeft = self:createObject("Multiply", "vcaLeft")
  connect(envelope, "Out",      vcaLeft, "Left")
  connect(filter,   "Left Out", vcaLeft, "Right")
  connect(vcaLeft,  "Out",      self,    "Out1")

  if channelCount > 1 then
    connect(self, "In2", filter, "Right In")

    local vcaRight = self:createObject("Multiply", "vcaRight")
    connect(envelope, "Out",       vcaRight, "Left")
    connect(filter,   "Right Out", vcaRight, "Right")
    connect(vcaRight, "Out",       self,     "Out2")
  end
end

function Strike:onLoadViews(objects, branches)
  local controls, views = {}, {
    expanded  = { "strike", "loop", "lift", "time", "curve" },
    collapsed = { "loop", "lift", "time" },

    strike    = { "wave3", "strike", "loop" },
    loop      = { "wave3", "strike", "loop" },
    lift      = { "wave2", "lift", "peak", "Q" },
    time      = { "wave2", "time", "attack", "decay" },
    curve     = { "wave2", "curve", "curveIn", "curveOut" }
  }

  local createMap = function (min, max, superCourse, course, fine, superFine, rounding)
    local map = app.LinearDialMap(min, max)
    map:setSteps(superCourse, course, fine, superFine)
    map:setRounding(rounding)
    return map
  end

  local fineMap     = createMap(0, 1, 0.1, 0.01, 0.001, 0.001, 0.001)
  local fineMapTime = createMap(0, 10, 0.1, 0.01, 0.001, 0.001, 0.001)

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
    description = "Keep Going!",
    branch      = branches.loop,
    comparator  = objects.loop
  }

  controls.lift = GainBias {
    button        = "lift",
    description   = "How Bright?",
    branch        = branches.lift,
    gainbias      = objects.lift,
    range         = objects.liftRange,
    biasMap       = fineMap,
    biasUnits     = app.unitNone,
    biasPrecision = 3,
    initialBias   = config.initialLift
  }

  controls.peak = GainBias {
    button      = "peak",
    branch      = branches.peak,
    description = "Peak Frequency",
    gainbias    = objects.peak,
    range       = objects.peakRange,
    biasMap     = Encoder.getMap("filterFreq"),
    biasUnits   = app.unitHertz,
    initialBias = config.initialPeak,
    gainMap     = Encoder.getMap("freqGain"),
    scaling     = app.octaveScaling
  }

  controls.Q = GainBias {
    button        = "Q",
    description   = "Peak Resonance",
    branch        = branches.Q,
    gainbias      = objects.Q,
    range         = objects.QRange,
    biasMap       = fineMap,
    biasUnits     = app.unitNone,
    biasPrecision = 3,
    initialBias   = config.initialQ
  }

  controls.time = GainBias {
    button        = "time",
    description   = "How Long?",
    branch        = branches.time,
    gainbias      = objects.time,
    range         = objects.timeRange,
    biasMap       = fineMap,
    biasUnits     = app.unitNone,
    biasPrecision = 3,
    initialBias   = config.initialTime
  }

  controls.attack = GainBias {
    button        = "attack",
    description   = "Attack Time",
    branch        = branches.attack,
    gainbias      = objects.attack,
    range         = objects.attackRange,
    biasMap       = fineMapTime,
    biasUnits     = app.unitSecs,
    biasPrecision = 3,
    initialBias   = config.initialAttack
  }

  controls.decay = GainBias {
    button        = "decay",
    description   = "Decay Time",
    branch        = branches.decay,
    gainbias      = objects.decay,
    range         = objects.decayRange,
    biasMap       = fineMapTime,
    biasUnits     = app.unitSecs,
    biasPrecision = 3,
    initialBias   = config.initialDecay
  }

  controls.curve = GainBias {
    button        = "curve",
    description   = "How Curved?",
    branch        = branches.curve,
    gainbias      = objects.curve,
    range         = objects.curveRange,
    biasMap       = fineMap,
    biasPrecision = 3,
    initialBias   = config.initialCurve
  }

  controls.curveIn = GainBias {
    button        = "curveIn",
    description   = "Attack Curve",
    branch        = branches.curveIn,
    gainbias      = objects.curveIn,
    range         = objects.curveInRange,
    biasMap       = fineMap,
    biasPrecision = 3,
    initialBias   = config.initialCurveIn
  }

  controls.curveOut = GainBias {
    button        = "curveOut",
    description   = "Decay Curve",
    branch        = branches.curveOut,
    gainbias      = objects.curveOut,
    range         = objects.curveOutRange,
    biasMap       = fineMap,
    biasPrecision = 3,
    initialBias   = config.initialCurveOut
  }

  return controls, views
end

return Strike