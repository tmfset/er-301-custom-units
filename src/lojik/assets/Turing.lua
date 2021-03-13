local app = app
local lojik = require "lojik.liblojik"
local core = require "core.libcore"
local Class = require "Base.Class"
local Unit = require "Unit"
local Encoder = require "Encoder"
local Gate = require "Unit.ViewControl.Gate"
local GainBias = require "Unit.ViewControl.GainBias"
local PitchCircle = require "core.Quantizer.PitchCircle"
local Common = require "lojik.Common"
local ply = app.SECTION_PLY

local Turing = Class {}
Turing:include(Common)

function Turing:init(args)
  args.title = "Turing"
  args.mnemonic = "TM"
  self.max = 16
  Unit.init(self, args)
end

function Turing:onLoadGraph(channelCount)
  local step    = self:addComparatorControl("step",  app.COMPARATOR_TRIGGER_ON_RISE)
  local write   = self:addComparatorControl("write", app.COMPARATOR_TOGGLE, 1)
  local length  = self:addGainBiasControl("length")

  local shift   = self:addComparatorControl("shift", app.COMPARATOR_GATE)
  local reset   = self:addComparatorControl("reset", app.COMPARATOR_GATE)

  local gain    = self:addGainBiasControl("gain")
  local scatter = self:addComparatorControl("scatter", app.COMPARATOR_TOGGLE, 1)

  local register = self:addObject("register", lojik.Register(16))
  connect(self,    "In1", register, "In")
  connect(step,    "Out", register, "Clock")
  connect(write,   "Out", register, "Capture")
  connect(length,  "Out", register, "Length")
  connect(shift,   "Out", register, "Shift")
  connect(reset,   "Out", register, "Reset")
  connect(gain,    "Out", register, "Gain")
  connect(scatter, "Out", register, "Scatter")

  local quantizer = self:addObject("quantizer", core.ScaleQuantizer())
  connect(register,  "Out", quantizer, "In")
  connect(quantizer, "Out", self,      "Out1")

  if channelCount >= 2 then
    for i = 2, channelCount do
      -- We can't really make multiple quantizers since each has to be passed
      -- into its own PitchCircle, so just connect the first output into any
      -- other channels.
      connect(quantizer, "Out", self, "Out"..i)
    end
  end
end

function Turing:makeGateViewF(objects, branches)
  return function (name, description)
    return Gate {
      button      = name,
      description = description,
      branch      = branches[name],
      comparator  = objects[name]
    }
  end
end

function Turing:onLoadViews(objects, branches)
  local makeGateView = self:makeGateViewF(objects, branches)

  local intMap = function (min, max)
    local map = app.LinearDialMap(min,max)
    map:setSteps(1, 1, 1, 1)
    map:setRounding(1)
    return map
  end

  return {
    step    = makeGateView("step", "Advance"),
    write   = makeGateView("write", "Enable Write"),
    shift   = makeGateView("shift", "Enable Shift"),
    reset   = makeGateView("reset", "Enable Reset"),
    scatter = makeGateView("scatter", "Enable Scatter"),
    length = GainBias {
      button        = "length",
      description   = "Length",
      branch        = branches.length,
      gainbias      = objects.length,
      range         = objects.lengthRange,
      gainMap       = intMap(-self.max, self.max),
      biasMap       = intMap(1, self.max),
      biasPrecision = 0,
      initialBias   = 4
    },
    gain = GainBias {
      button        = "gain",
      description   = "Input Gain",
      branch        = branches.gain,
      gainbias      = objects.gain,
      range         = objects.gainRange,
      biasMap       = Encoder.getMap("[0,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 3,
      initialBias   = 0.125
    },
    scale = PitchCircle {
      name      = "scale",
      width     = 2 * ply,
      quantizer = objects.quantizer
    }
  }, {
    expanded  = { "step", "write", "length", "scale" },
    step      = { "step", "shift", "reset" },
    write     = { "write", "gain", "scatter" },
    collapsed = { "scale" }
  }
end

return Turing
