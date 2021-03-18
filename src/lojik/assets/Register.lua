local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local Encoder = require "Encoder"
local GainBias = require "Unit.ViewControl.GainBias"
local MenuHeader = require "Unit.MenuControl.Header"
local Task = require "Unit.MenuControl.Task"
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
  local stride  = self:addGainBiasControl("stride")

  local shift   = self:addComparatorControl("shift", app.COMPARATOR_GATE)
  local reset   = self:addComparatorControl("reset", app.COMPARATOR_GATE)

  local gain    = self:addGainBiasControl("gain")
  local scatter = self:addComparatorControl("scatter", app.COMPARATOR_TOGGLE, 1)

  for i = 1, channelCount do
    local register = self:addObject("register"..i, lojik.Register(self.max, true))
    connect(self,     "In"..i, register, "In")

    connect(step,     "Out",   register, "Clock")
    connect(write,    "Out",   register, "Capture")
    connect(length,   "Out",   register, "Length")
    connect(stride,   "Out",   register, "Stride")
    connect(shift,    "Out",   register, "Shift")
    connect(reset,    "Out",   register, "Reset")
    connect(gain,     "Out",   register, "Gain")
    connect(scatter,  "Out",   register, "Scatter")

    connect(register, "Out",   self,     "Out"..i)
  end
end

function Register:serialize()
  local t = Unit.serialize(self)
  t.registers = {}

  for i = 1, self.channelCount do
    local register = self.objects["register"..i]
    if register then
      t.registers[i] = self.serializeRegister(register)
    end
  end

  return t
end

function Register:deserialize(t)
  Unit.deserialize(self, t)

  for i, rt in ipairs(t.registers or {}) do
    local register = self.objects["register"..i]
    if register then
      self.deserializeRegister(register, rt)
    end
  end
end

function Register:onShowMenu()
  return {
    zeroHeader = MenuHeader {
      description = "Zero!"
    },
    zeroAll = Task {
      description = "All",
      task = function ()
        for i = 1, self.channelCount do
          self.objects["register"..i]:triggerZeroAll()
        end
      end
    },
    zeroWindow = Task {
      description = "Window",
      task = function ()
        for i = 1, self.channelCount do
          self.objects["register"..i]:triggerZeroWindow()
        end
      end
    },
    randomizeHeader = MenuHeader {
      description = "Randomize!"
    },
    randomizeAll = Task {
      description = "All",
      task = function ()
        for i = 1, self.channelCount do
          self.objects["register"..i]:triggerRandomizeAll()
        end
      end
    },
    randomizeWindow = Task {
      description = "Window",
      task = function ()
        for i = 1, self.channelCount do
          self.objects["register"..i]:triggerRandomizeAll()
        end
      end
    }
  }, {
    "zeroHeader", "zeroAll", "zeroWindow",
    "randomizeHeader", "randomizeAll", "randomizeWindow"
  }
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
    stride  = GainBias {
      button        = "stride",
      description   = "Stride",
      branch        = self.branches.stride,
      gainbias      = self.objects.stride,
      range         = self.objects.strideRange,
      gainMap       = self.intMap(-self.max / 4, self.max / 4),
      biasMap       = self.intMap(-self.max / 4, self.max / 4),
      biasPrecision = 0,
      initialBias   = 1
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
    expanded  = { "step", "write", "length", "stride", "gain" },
    step      = { "step", "shift", "reset" },
    write     = { "write", "gain", "scatter" },
    collapsed = { "length" }
  }
end

return Register
