local app = app
local lojik = require "lojik.liblojik"
local core = require "core.libcore"
local Class = require "Base.Class"
local Unit = require "Unit"
local Encoder = require "Encoder"
local GainBias = require "Unit.ViewControl.GainBias"
local PitchCircle = require "core.Quantizer.PitchCircle"
local Common = require "lojik.Common"
local MenuHeader = require "Unit.MenuControl.Header"
local Task = require "Unit.MenuControl.Task"
local ply = app.SECTION_PLY

local Turing = Class {}
Turing:include(Unit)
Turing:include(Common)

function Turing:init(args)
  args.title = "Turing"
  args.mnemonic = "TM"
  self.max = 16
  Unit.init(self, args)
end

function Turing:onLoadGraph(channelCount)
  local step    = self:addComparatorControl("step",  app.COMPARATOR_TRIGGER_ON_RISE)
  local write   = self:addComparatorControl("write", app.COMPARATOR_TOGGLE)
  local length  = self:addGainBiasControl("length")
  local stride  = self:addGainBiasControl("stride")

  local shift   = self:addComparatorControl("shift", app.COMPARATOR_GATE)
  local reset   = self:addComparatorControl("reset", app.COMPARATOR_GATE)

  local gain    = self:addGainBiasControl("gain")
  local scatter = self:addComparatorControl("scatter", app.COMPARATOR_TOGGLE, 1)

  local register = self:addObject("register", lojik.Register(self.max, true))
  connect(self,    "In1", register, "In")
  connect(step,    "Out", register, "Clock")
  connect(write,   "Out", register, "Capture")
  connect(length,  "Out", register, "Length")
  connect(stride,  "Out", register, "Stride")
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

function Turing:serialize()
  local t = Unit.serialize(self)

  t.registers = {}
  t.registers[1] = self.serializeRegister(self.objects.register)

  return t
end

function Turing:deserialize(t)
  Unit.deserialize(self, t)

  local register = self.objects.register;
  local rt = t.registers[1];
  if rt then
    self.deserializeRegister(register, rt)
  end
end

function Turing:onShowMenu()
  return {
    zeroHeader = MenuHeader {
      description = "Zero!"
    },
    zeroAll = Task {
      description = "All",
      task = function ()
        self.objects.register:triggerZeroAll()
      end
    },
    zeroWindow = Task {
      description = "Window",
      task = function ()
        self.objects.register:triggerZeroWindow()
      end
    },
    randomizeHeader = MenuHeader {
      description = "Randomize!"
    },
    randomizeAll = Task {
      description = "All",
      task = function ()
        self.objects.register:triggerRandomizeAll()
      end
    },
    randomizeWindow = Task {
      description = "Window",
      task = function ()
        self.objects.register:triggerRandomizeAll()
      end
    }
  }, {
    "zeroHeader", "zeroAll", "zeroWindow",
    "randomizeHeader", "randomizeAll", "randomizeWindow"
  }
end


function Turing:onLoadViews()
  return {
    step    = self:gateView("step", "Advance"),
    write   = self:gateView("write", "Enable Write"),
    shift   = self:gateView("shift", "Enable Shift"),
    reset   = self:gateView("reset", "Enable Reset"),
    scatter = self:gateView("scatter", "Enable Scatter"),
    length = GainBias {
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
      gainMap       = self.intMap(-self.max, self.max),
      biasMap       = self.intMap(1, self.max),
      biasPrecision = 0,
      initialBias   = 1
    },
    gain = GainBias {
      button        = "gain",
      description   = "Input Gain",
      branch        = self.branches.gain,
      gainbias      = self.objects.gain,
      range         = self.objects.gainRange,
      biasMap       = Encoder.getMap("[0,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 3,
      initialBias   = 0.125
    },
    scale = PitchCircle {
      name      = "scale",
      width     = 2 * ply,
      quantizer = self.objects.quantizer
    }
  }, {
    expanded  = { "step", "write", "length", "scale" },
    step      = { "step", "shift", "reset", "stride" },
    write     = { "write", "gain", "scatter" },
    collapsed = { "scale" }
  }
end

return Turing
