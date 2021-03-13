local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local Encoder = require "Encoder"
local GainBias = require "Unit.ViewControl.GainBias"
local Common = require "lojik.Common"

local Register = Class {}
Register:include(Unit)
Register:include(Common)

function Register:init(args)
  args.title = "Register"
  args.mnemonic = "R"
  self.max = 64
  Unit.init(self, args)
end

function Register:onLoadGraph(channelCount)
  local step    = self:addComparatorControl("step",  app.COMPARATOR_TRIGGER_ON_RISE)
  local write   = self:addComparatorControl("write", app.COMPARATOR_TOGGLE)
  local length  = self:addGainBiasControl("length")

  local shift   = self:addComparatorControl("shift", app.COMPARATOR_GATE)
  local reset   = self:addComparatorControl("reset", app.COMPARATOR_GATE)

  local gain    = self:addGainBiasControl("gain")
  local scatter = self:addComparatorControl("scatter", app.COMPARATOR_TOGGLE, 1)

  for i = 0, channelCount do
    local register = self:addObject("register"..i, lojik.Register(self.max))
    connect(self,     "In"..i, register, "In")

    connect(step,     "Out",   register, "Clock")
    connect(write,    "Out",   register, "Capture")
    connect(length,   "Out",   register, "Length")
    connect(shift,    "Out",   register, "Shift")
    connect(reset,    "Out",   register, "Reset")
    connect(gain,     "Out",   register, "Gain")
    connect(scatter,  "Out",   register, "Scatter")

    connect(register, "Out",   self,     "Out"..i)
  end
end

function Register:onLoadViews()
  return {
    step    = self:gateView("step", "Advance"),
    write   = self:gateView("write", "Enable Write"),
    shift   = self:gateView("shift", "Enable Shift"),
    reset   = self:gateView("reset", "Enable Reset"),
    scatter = self:gateView("scatter", "Enable Scatter"),
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
    gain   = GainBias {
      button        = "gain",
      description   = "Input Gain",
      branch        = self.branches.gain,
      gainbias      = self.objects.gain,
      range         = self.objects.gainRange,
      biasMap       = Encoder.getMap("[0,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 1
    }
  }, {
    expanded  = { "step", "write", "length", "gain" },
    step      = { "step", "shift", "reset" },
    write     = { "write", "gain", "scatter" },
    collapsed = { "length" }
  }
end

return Register
