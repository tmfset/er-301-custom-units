local app = app
local core = require "core.libcore"
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local Encoder = require "Encoder"
local MenuHeader = require "Unit.MenuControl.Header"
local Task = require "Unit.MenuControl.Task"
local FlagSelect = require "Unit.MenuControl.FlagSelect"
local PitchCircle = require "core.Quantizer.PitchCircle"
local GainBias = require "Unit.ViewControl.GainBias"
local OutputScope = require "Unit.ViewControl.OutputScope"
local Common = require "lojik.Common"

local Seq = Class {}
Seq:include(Unit)
Seq:include(Common)

function Seq:init(args)
  args.title = "Seq"
  args.mnemonic = "S"
  self.max = 64
  Unit.init(self, args)
end

function Seq:onLoadGraph(channelCount)
  local clock   = self:addComparatorControl("clock",  app.COMPARATOR_TRIGGER_ON_RISE)
  local capture = self:addComparatorControl("capture", app.COMPARATOR_GATE)
  local stride  = self:addGainBiasControl("stride")

  local shift   = self:addComparatorControl("shift", app.COMPARATOR_TRIGGER_ON_RISE)
  local reset   = self:addComparatorControl("reset", app.COMPARATOR_TRIGGER_ON_RISE)

  local scatter = self:addParameterAdapterControl("scatter")
  local drift   = self:addParameterAdapterControl("drift")
  local gain    = self:addParameterAdapterControl("gain")
  local bias    = self:addParameterAdapterControl("bias")

  local register = self:addObject("register", lojik.Register(self.max, 0))
  register:setOptionValue("Mode", lojik.MODE_SEQ)
  register:getOption("Sync"):clearFlag(1) -- Capture

  connect(self, "In1", register, "In")

  connect(clock,   "Out", register, "Clock")
  connect(capture, "Out", register, "Capture")
  connect(stride,  "Out", register, "Stride")
  connect(shift,   "Out", register, "Shift")
  connect(reset,   "Out", register, "Reset")

  tie(register, "Scatter",    scatter, "Out")
  tie(register, "Drift",      drift,   "Out")
  tie(register, "Input Gain", gain,    "Out")
  tie(register, "Input Bias", bias,    "Out")

  local quantizer = self:addObject("quantizer", core.ScaleQuantizer())
  connect(register, "Out", quantizer, "In")

  for i = 1, channelCount do
    connect(quantizer, "Out", self, "Out"..i)
  end
end

function Seq:serialize()
  local t = Unit.serialize(self)

  t.registers = {}
  t.registers[1] = self.serializeRegister(self.objects.register)

  return t
end

function Seq:deserialize(t)
  Unit.deserialize(self, t)

  local register = self.objects.register;
  local rt = t.registers[1];
  if rt then
    self.deserializeRegister(register, rt)
  end
end

function Seq:onShowMenu(objects)
  return {
    windowHeader = MenuHeader {
      description = "Set window (" .. objects.register:getLength() .. ") ..."
    },
    zeroWindow = Task {
      description = "zero",
      task = function ()
        objects.register:triggerZeroWindow()
      end
    },
    scatterWindow = Task {
      description = "scatter",
      task = function ()
        objects.register:triggerScatterWindow()
      end
    },
    randomizeWindow = Task {
      description = "zero + scatter",
      task = function ()
        objects.register:triggerRandomizeAll()
      end
    },
    allHeader = MenuHeader {
      description = "Set all (" .. objects.register:getMax() .. ") ..."
    },
    zeroAll = Task {
      description = "zero",
      task = function ()
        objects.register:triggerZeroAll()
      end
    },
    scatterAll = Task {
      description = "scatter",
      task = function ()
        objects.register:triggerScatterAll()
      end
    },
    randomizeAll = Task {
      description = "zero + scatter",
      task = function ()
        objects.register:triggerRandomizeAll()
      end
    },
    clockSync = FlagSelect {
      description = "Clock Sync",
      option      = objects.register:getOption("Sync"),
      flags       = { "shift", "capture", "reset" }
    }
  }, {
    "clockSync",
    "allHeader",    "zeroAll",    "scatterAll",    "randomizeAll",
    "windowHeader", "zeroWindow", "scatterWindow", "randomizeWindow"
  }
end

function Seq:onLoadViews()
  return {
    wave1 = OutputScope {
      monitor = self,
      width   = 1 * app.SECTION_PLY
    },
    wave3 = OutputScope {
      monitor = self,
      width   = 3 * app.SECTION_PLY
    },
    clock   = self:gateView("clock", "Advance"),
    capture = self:gateView("capture", "Enable Write"),
    shift   = self:gateView("shift", "Enable Shift"),
    reset   = self:gateView("reset", "Enable Reset"),
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
    scatter   = GainBias {
      button        = "scatter",
      description   = "Scatter",
      branch        = self.branches.scatter,
      gainbias      = self.objects.scatter,
      range         = self.objects.scatter,
      biasMap       = Encoder.getMap("[0,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0
    },
    drift   = GainBias {
      button        = "drift",
      description   = "Drift",
      branch        = self.branches.drift,
      gainbias      = self.objects.drift,
      range         = self.objects.drift,
      biasMap       = Encoder.getMap("[0,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0
    },
    gain   = GainBias {
      button        = "gain",
      description   = "Input Gain",
      branch        = self.branches.gain,
      gainbias      = self.objects.gain,
      range         = self.objects.gain,
      biasMap       = Encoder.getMap("[0,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 1
    },
    bias   = GainBias {
      button        = "bias",
      description   = "Input Bias",
      branch        = self.branches.bias,
      gainbias      = self.objects.bias,
      range         = self.objects.bias,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0
    },
    scale = PitchCircle {
      name      = "scale",
      width     = 1 * app.SECTION_PLY,
      quantizer = self.objects.quantizer
    }
  }, {
    expanded  = { "clock", "capture", "reset", "scale" },

    clock     = { "clock",   "wave1", "shift", "reset" },
    capture   = { "capture", "wave1", "gain", "bias", "scatter" },
    reset     = { "reset",   "wave1", "stride", "drift" },
    scale     = { "scale",   "wave3" },

    collapsed = { "scale" }
  }
end

return Seq
