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

local Polygon = Class {}
Polygon:include(Unit)

function Polygon:init(args)
  self.voiceCount = args.voiceCount or app.error("%s.init: voiceCount is missing.", self)

  self.oscillatorMeta = {
    self:createOscillatorMeta("A", "Square"),
    self:createOscillatorMeta("B", "Saw"),
    self:createOscillatorMeta("C", "Triangle")
  }

  args.version = 1
  Unit.init(self, args)
end

-- #### Utility ###############################################################

-- Create a memoized constant value to be used as an output.
function Polygon:mConst(value)
  self.constants = self.constants or {}

  if self.constants[value] == nil then
    local const = self:createObject("Constant", "constant"..value)
    const:hardSet("Value", value)

    self.constants[value] = const
  end

  return self.constants[value]
end

-- Add together two outputs.
function Polygon:sum(left, right, suffix)
  local sum = self:createObject("Sum", "sum"..suffix)
  connect(left, "Out", sum, "Left")
  connect(right, "Out", sum, "Right")
  return sum
end

-- Multiply two outputs.
function Polygon:mult(left, right, suffix)
  local mult = self:createObject("Multiply", "vca"..suffix)
  connect(left, "Out", mult, "Left")
  connect(right, "Out", mult, "Right")
  return mult
end

-- Track this with that
function Polygon:track(this, that, suffix)
  local track = self:createObject("TrackAndHold", "track"..suffix)
  connect(this, "Out", track, "In")
  connect(that, "Out", track, "Track")
  return track
end


-- Create a mix of the given objects that will be automatically gain controlled
-- based on their collective levels.
function Polygon:agc(suffix, objects)
  local count    = #objects
  local mix      = self:createObject("Mixer", "mixer"..suffix, count)
  local levelSum = self:createObject("Mixer", "levelSum"..suffix, count)
  local agc      = self:createObject("RationalMultiply", "agc"..suffix)
  local fullMix  = self:createObject("Mixer", "fullMix"..suffix, count)

  local env = self:createObject("EnvelopeFollower","env")
  env:hardSet("Attack Time", 0.1)
  env:hardSet("Release Time", 0.2)
  connect(fullMix, "Out", env, "In")

  local ratio = self:createObject("RationalMultiply", "ratio"..suffix)
  connect(self:mConst(1), "Out", ratio, "In")
  connect(env,            "Out", ratio, "Numerator")
  connect(levelSum,       "Out", ratio, "Divisor")

  local inv    = self:mult(self:mConst(count), self:mConst(-1), "inv"..suffix)
  local diff   = self:sum(inv, levelSum, "diff"..suffix)
  local scaled = self:mult(diff, ratio, "scaled"..suffix)
  local div    = self:sum(scaled, self:mConst(count), "div"..suffix)

  connect(mix,      "Out", agc, "In")
  connect(self:mConst(count), "Out", agc, "Numerator")
  connect(div,      "Out", agc, "Divisor")

  for i = 1, count do
    connect(objects[i].out,   "Out", fullMix,  "In"..i)
    connect(objects[i].out,   "Out", mix,      "In"..i)
    connect(objects[i].level, "Out", levelSum, "In"..i)

    fullMix:hardSet("Gain"..i, 1)
    mix:hardSet("Gain"..i, 1 / count)
    levelSum:hardSet("Gain"..i, 1)
  end

  return agc
end

function Select(objects, key)
  local out = {}
  for i = 1, #objects do
    out[i] = objects[i][key]
  end
  return out
end

function Polygon:mix(suffix, level, objects)
  local mix = self:createObject("Mixer", "mixer"..suffix, #objects)
  for i = 1, #objects do
    connect(objects[i], "Out", mix, "In"..i)
    mix:hardSet("Gain"..i, level)
  end
  return mix
end

function Polygon:agc(suffix, objects)
  local count    = #objects
  local mix      = self:mix("CG"..suffix, 1 / count, Select(objects, "out"))
  local levelSum = self:mix("LS"..suffix, 1,         Select(objects, "level"))
  local agc      = self:createObject("RationalMultiply", "agc"..suffix)

  connect(mix,                "Out", agc, "In")
  connect(self:mConst(count), "Out", agc, "Numerator")
  connect(levelSum,           "Out", agc, "Divisor")

  return agc
end

function Polygon:roundRobin(suffix, total)
  local gate   = self:createGateControl("RR"..suffix)
  local sync   = self:createSyncControl("RR"..suffix)
  local tune   = self:createTuneControl("RR"..suffix)

  local count  = self:createObject("Counter", "RRCount"..suffix)
  count:hardSet("Start", 0)
  count:hardSet("Step Size", 1)
  count:hardSet("Finish", total - 1)
  count:hardSet("Gain", 1 / (total - 1))

  -- Create a switch to trigger the counter, inverting the gate since the
  -- Comparator triggerOnFallMode doesn't seem to work.
  local switch = self:createComparatorControl("RRSwitch"..suffix, 3)
  local invert = self:mult(gate, self:mConst(-1), "RRInvert"..suffix)
  switch:hardSet("Threshold", -0.5)
  connect(invert, "Out", switch, "In")
  connect(switch, "Out", count, "In")

  return function (step)
    return function (suffix, args)
      local isActive = self:createObject("BumpMap", "RRIsActive"..suffix)
      connect(count, "Out", isActive, "In")
      isActive:hardSet("Center", (step - 1) / (total - 1))
      isActive:hardSet("Width", 1 / ((total - 1) * 3))
      isActive:hardSet("Height", 1)
      isActive:hardSet("Fade", 0.001)

      local rrGate = self:mult(isActive, gate, "RRGate"..suffix)
      local cGate  = self:sum(rrGate, args.gate, "RRCombinedGate"..suffix)

      local rrSync = self:mult(isActive, sync, "RRSync"..suffix)
      local cSync  = self:sum(rrSync, args.sync, "RRCombinedSync"..suffix)

      local rrTune = self:mult(isActive, tune, "RRTune"..suffix)
      local track  = self:track(rrTune, cGate, "RRTune"..suffix)
      local cTune  = self:sum(track, args.tune, "RRCobinedTune"..suffix)

      return cGate, cSync, cTune
    end
  end
end

-- ############################################################################

-- Create a synth voice
function Polygon:createVoice(suffix, args)
  local gate, sync, tune = args.tieInputs(suffix, {
    gate = self:createGateControl(suffix),
    sync = self:createSyncControl(suffix),
    tune = self:createTuneControl(suffix)
  })

  local adsr = self:createObject("ADSR", "adsr"..suffix)
  connect(gate,              "Out", sync, "In")
  connect(gate,              "Out", adsr, "Gate")
  connect(args.adsr.attack,  "Out", adsr, "Attack")
  connect(args.adsr.decay,   "Out", adsr, "Decay")
  connect(args.adsr.sustain, "Out", adsr, "Sustain")
  connect(args.adsr.release, "Out", adsr, "Release")

  local oscs = {}
  for i = 1, #args.oscillators do
    local control = args.oscillators[i]
    oscs[i] = self:createOscillator(i..suffix, control.type, {
      f0    = control.f0,
      tune  = self:sum(tune, control.tune, i..suffix),
      level = control.level,
      sync  = sync,
      meta  = control.meta
    })
  end

  local filter  = self:createObject("StereoLadderFilter", "filter"..suffix)
  local fEnvVca = self:mult(adsr, args.filter.env, "fEnvVca"..suffix)
  connect(fEnvVca,               "Out", filter, "V/Oct")
  connect(args.filter.cutoff,    "Out", filter, "Fundamental")
  connect(args.filter.resonance, "Out", filter, "Resonance")

  local agc = self:agc(suffix, oscs)
  connect(agc, "Out", filter, "Left In")

  local vca = self:createObject("Multiply", "vca"..suffix)
  connect(adsr,   "Out",      vca, "Left")
  connect(filter, "Left Out", vca, "Right")

  return vca
end

-- #### Oscillators ###########################################################

function Polygon:createOscillatorMeta(name, type)
  local lut = {
    Square = function (suffix)
      return {
        pulseWidth = self:createControl("ParameterAdapter", "pulseWidth"..suffix)
      }
    end,
    Saw = function (suffix)
      return {
        phase = self:createControl("ParameterAdapter", "phase"..suffix)
      }
    end,
    Triangle = function (suffix)
      return {
        phase = self:createControl("ParameterAdapter", "phase"..suffix)
      }
    end,
    Sine = function (suffix)
      return {
        phase = self:createControl("GainBias", "phase"..suffix),
        feedback = self:createControl("GainBias", "feedback"..suffix)
      }
    end
  }

  local meta = lut[type](type..name)
  meta.type = type
  meta.name = name

  return meta
end

function Polygon:createOscillator(suffix, type, args)
  local standard = function (type)
    return function (suffix, args)
      local osc   = self:createObject(type, "osc"..suffix)
      local level = self:mult(args.level, osc, "level"..suffix)

      tie(osc, "Phase", args.meta.phase, "Out")

      connect(args.sync, "Out", osc, "Sync")
      connect(args.tune, "Out", osc, "V/Oct")
      connect(args.f0,   "Out", osc, "Fundamental")

      return { out = level, tune = args.tune, level = args.level }
    end
  end

  local lut = {
    Sine        = function (suffix, args)
      local osc   = self:createObject("SineOscillator", "osc"..suffix)
      local level = self:mult(args.level, osc, "level"..suffix)

      connect(args.meta.phase,    "Out", osc, "Phase")
      connect(args.meta.feedback, "Out", osc, "Feedback")
      connect(args.sync,          "Out", osc, "Sync")
      connect(args.tune,          "Out", osc, "V/Oct")
      connect(args.f0,            "Out", osc, "Fundamental")

      return { out = level, tune = args.tune, level = args.level }
    end,
    Triangle    = standard("TriangleOscillator"),
    Saw         = standard("SawtoothOscillator"),
    Square      = function (suffix, args)
      local clock    = self:createObject("ClockInHertz", "clock"..suffix)
      local offset   = self:sum(self:mConst(-0.5), clock, "offset"..suffix)
      local double   = self:mult(self:mConst(2), offset, "double"..suffix)
      local level    = self:mult(args.level, double, "level"..suffix)
      local freqAdpt = self:createObject("ParameterAdapter", "freqAdpt"..suffix)
      local gain     = self:createObject("Gain", "freqGain"..suffix)
      local vpo      = self:createObject("VoltPerOctave", "vpo"..suffix)

      freqAdpt:hardSet("Gain", 1)
      tie(clock, "Frequency", freqAdpt, "Out")
      tie(clock, "Pulse Width", args.meta.pulseWidth, "Out")

      connect(args.sync, "Out", clock,    "Sync")
      connect(args.tune, "Out", vpo,      "In")
      connect(vpo,       "Out", gain,     "In")
      connect(args.f0,   "Out", gain,     "Gain")
      connect(gain,      "Out", freqAdpt, "In")

      return { out = level, tune = args.tune, level = args.level }
    end,
    SingleCycle = function (suffix, args)
      local osc = self:createObject("SingleCycle", "osc"..suffix)
      connect(args.f0, "Out", osc, "Fundamental")

      local scan = self:createOffsetControl(args.scan, "scan"..suffix)
      local phase = self:createOffsetControl(args.phase, "scan"..suffix)

      return { out = osc, tune = args.tune, scan = scan, phase = phase }
    end
  }

  return lut[type](suffix, args)
end

function Polygon:createOscillatorUI()
end

-- ############################################################################

function Polygon:onLoadGraph(channelCount)
  local f0 = self:createControl("GainBias", "f0")

  local oscillators = {}
  for i = 1, #self.oscillatorMeta do
    oscillators[i] = {
      f0    = f0,
      level = self:createLevelControl(self.oTypes[i].name),
      tune  = self:createTuneControl(self.oTypes[i].name),
      meta  = self.oscillatorMeta[i]
    }
  end

  local adsr = {
    attack  = self:createControl("GainBias", "attack"),
    decay   = self:createControl("GainBias", "decay"),
    sustain = self:createControl("GainBias", "sustain"),
    release = self:createControl("GainBias", "release")
  }

  local filter = {
    env       = self:createControl("GainBias", "fEnv"),
    cutoff    = self:createControl("GainBias", "cutoff"),
    resonance = self:createControl("GainBias", "resonance")
  }

  local roundRobin = self:roundRobin("", self.voiceCount)
  local voices = {}
  for i = 1, self.voiceCount do
    voices[i] = self:createVoice(i, {
      oscillators = oscillators,
      adsr        = adsr,
      filter      = filter,
      tieInputs   = roundRobin(i)
    })
  end

  local mix     = self:mix("All", 1 / #voices, voices)
  local gain    = self:createControl("ConstantGain", "gain")
  local limiter = self:createControl("Limiter", "limiter")
  gain:hardSet("Gain", 1)
  limiter:setOptionValue("Type", 3)
  connect(mix,  "Out", gain,    "In")
  connect(gain, "Out", limiter, "In")

  local level = self:createLevelControl("")
  local out = self:mult(limiter, level, "Output")
  connect(out, "Out", self, "Out1")
end

function Polygon:createControl(type, name)
  local control = self:createObject(type, name)
  local controlRange = self:createObject("MinMax", name.."Range")
  connect(control, "Out", controlRange, "In")
  self:createMonoBranch(name, control, "In", control, "Out")
  return control
end

function Polygon:createGainBiasUI(objects, branches)
  return function (name, description, suffix, bias, root, map, units)
    return GainBias {
      button      = name,
      description = description,
      branch      = branches[root..suffix],
      gainbias    = objects[root..suffix],
      range       = objects[root..suffix.."Range"],
      biasMap     = Encoder.getMap(map),
      biasUnits   = units,
      initialBias = bias
    }
  end
end

function Polygon:createTuneControl(suffix)
  return self:createControl("ConstantOffset", "tune"..suffix)
end

function Polygon:createTuneUI(objects, branches)
  return function (name, description, suffix)
    return Pitch {
      button      = name,
      description = description,
      branch      = branches["tune"..suffix],
      offset      = objects["tune"..suffix],
      range       = objects["tune"..suffix.."Range"]
    }
  end
end

function Polygon:createComparatorControl(name, mode)
  local gate = self:createObject("Comparator", name)
  gate:setMode(mode)
  self:createMonoBranch(name, gate, "In", gate, "Out")
  return gate
end

function Polygon:createComparatorUI(objects, branches, type)
  return function (name, description, suffix)
    return Gate {
      button      = name,
      description = description,
      branch      = branches[type..suffix],
      comparator  = objects[type..suffix]
    }
  end
end

function Polygon:createGateControl(suffix)
  return self:createComparatorControl("gate"..suffix, 2)
end

function Polygon:createSyncControl(suffix)
  return self:createComparatorControl("sync"..suffix, 3)
end

function Polygon:createLevelControl(suffix)
  return self:createControl("GainBias", "level"..suffix)
end

function Polygon:createLevelUI(objects, branches)
  return function (name, description, index, bias)
    return self:createGainBiasUI(objects, branches)(name, description, index, bias, "level", "[0,1]", app.unitNone)
  end
end

function Polygon:createScanUI(objects, branches)
  return function (name, description, index)
    return self:createGainBiasUI(objects, branches)(name, description, index, 0, "scan", "[-1,1]", app.unitNone)
  end
end

function Polygon:createPhaseUI(objects, branches)
  return function (name, description, index)
    return self:createGainBiasUI(objects, branches)(name, description, index, 0, "phase", "[-1,1]", app.unitNone)
  end
end

function Polygon:onLoadViews(objects, branches)
  local controls = {}
  local views = { expanded = {}, collapsed = { "scope" } }

  controls.scope = OutputScope {
    monitor = self,
    width = 4 * ply
  }

  local gateUI  = self:createComparatorUI(objects, branches, "gate")
  local syncUI  = self:createComparatorUI(objects, branches, "sync")
  local tuneUI  = self:createTuneUI(objects, branches)
  local levelUI = self:createLevelUI(objects, branches)
  local gainUI  = self:createGainBiasUI(objects, branches)

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

  controls.gain = Fader {
    button = "gain",
    description = "Pre Limiter Gain",
    param = objects.gain:getParameter("Gain"),
    map = Encoder.getMap("decibel36"),
    units = app.unitDecibels
  }
  views.expanded[#views.expanded + 1] = "gain"

  controls.level = levelUI("level", "Overall Level", "", 1)
  views.expanded[#views.expanded + 1] = "level"
  views.level = { "level" }

  controls.gateRR = gateUI("gate", "Round Robin Gate", "RR")
  views.expanded[#views.expanded + 1] = "gateRR"
  views.gateRR = { "gateRR" }

  controls.syncRR = syncUI("sync", "Round Robin Sync", "RR")
  views.expanded[#views.expanded + 1] = "syncRR"
  views.syncRR = { "syncRR" }

  controls.tuneRR = tuneUI("V/Oct", "Round Robin V/Oct", "RR")
  views.expanded[#views.expanded + 1] = "tuneRR"
  views.tuneRR = { "tuneRR" }

  for i = 1, self.voiceCount do
    controls["gate"..i] = gateUI("gate"..i, "Voice "..i.." Gate", i)
    views.gateRR[#views.gateRR + 1] = "gate"..i

    controls["sync"..i] = syncUI("sync"..i, "Voice "..i.." Sync", i)
    views.syncRR[#views.syncRR + 1] = "sync"..i

    controls["tune"..i] = tuneUI("V/Oct"..i, "Voice "..i.." V/Oct ", i)
    views.tuneRR[#views.tuneRR + 1] = "tune"..i
  end

  for i = 1, #self.oscillatorMeta do
    local meta = self.oscillatorMeta[i]
    local name = meta.name

    controls["level"..name] = levelUI("level"..name, "Osc "..name.." Level", name, i == 1 and 1 or 0)
    views.level[#views.level + 1] = "level"..name

    controls["tune"..name] = tuneUI("V/Oct"..name, "Osc "..name.." V/Oct", name)
    views.tuneRR[#views.tuneRR + 1] = "tune"..name
  end

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

  controls.resonance = GainBias {
    button      = "Q",
    branch      = branches.resonance,
    description = "Resonance",
    gainbias    = objects.resonance,
    range       = objects.resonanceRange,
    biasMap     = Encoder.getMap("unit"),
    biasUnits   = app.unitNone,
    initialBias = 0,
    gainMap     = Encoder.getMap("[-10,10]")
  }

  controls.fEnv = GainBias {
    button      = "fenv",
    description = "Filter Env Amount",
    branch      = branches.fEnv,
    gainbias    = objects.fEnv,
    range       = objects.fEnvRange,
    biasMap     = Encoder.getMap("[-1,1]"),
    initialBias = 0.2
  }

  views.expanded[#views.expanded + 1] = "cutoff"
  views.expanded[#views.expanded + 1] = "fEnv"

  views.cutoff = { "cutoff", "resonance", "fEnv" }
  views.fEnv   = views.cutoff

  controls.attack  = gainUI("A", "Attack",  "", 0.05, "attack",  "ADSR", app.unitSecs)
  controls.decay   = gainUI("D", "Decay",   "", 0.05, "decay",   "ADSR", app.unitSecs)
  controls.sustain = gainUI("S", "Sustain", "", 1,    "sustain", "unit", app.unitNone)
  controls.release = gainUI("R", "Release", "", 1,    "release", "ADSR", app.unitSecs)

  views.release = { "attack", "decay", "sustain", "release" }
  views.expanded[#views.expanded + 1] = "release"

  return controls, views
end

return function (title, mnemonic, voiceCount)
  local Custom = Class {}
  Custom:include(Polygon)

  function Custom:init(args)
    args.title    = title
    args.mnemonic = mnemonic
    args.voiceCount = voiceCount

    Polygon.init(self, args)
  end

  return Custom
end