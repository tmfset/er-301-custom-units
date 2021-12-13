local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local Encoder = require "Encoder"
local GainBias = require "Unit.ViewControl.GainBias"
local MenuHeader = require "Unit.MenuControl.Header"
local Task = require "Unit.MenuControl.Task"
local FlagSelect = require "Unit.MenuControl.FlagSelect"
local OutputScope = require "Unit.ViewControl.OutputScope"
local UnitShared = require "common.assets.UnitShared"
local RegisterShared = require "lojik.RegisterShared"
local RegisterView = require "lojik.RegisterView"

local Register = Class {}
Register:include(Unit)
Register:include(UnitShared)
Register:include(RegisterShared)

function Register:init(args)
  args.title = "Register"
  args.mnemonic = "R"
  self.max = 128
  self.initialScatter = 0.5
  self.initialGain = 1.0
  self.initialBias = 0.0
  self.initialDrift = 0.0
  Unit.init(self, args)
end

function Register:onLoadGraph(channelCount)
  --local clock   = self:addComparatorControl("clock",  app.COMPARATOR_TRIGGER_ON_RISE)
  local captureGate = self:addComparatorControl("captureGate", app.COMPARATOR_TOGGLE)
  local shiftGate   = self:addComparatorControl("shiftGate", app.COMPARATOR_GATE)
  local resetGate   = self:addComparatorControl("resetGate", app.COMPARATOR_GATE)

  local offset  = self:addParameterAdapterControl("offset")
  local shift   = self:addParameterAdapterControl("shift")
  local length  = self:addParameterAdapterControl("length")
  local stride  = self:addParameterAdapterControl("stride")


  local outputGain = self:addParameterAdapterControl("outputGain")
  local outputBias = self:addParameterAdapterControl("outputBias")
  local inputGain  = self:addParameterAdapterControl("inputGain")
  local inputBias  = self:addParameterAdapterControl("inputBias")

  local register = self:addObject("register", lojik.Register2())
  connect(self, "In1", register, "Clock")
  connect(captureGate, "Out", register, "Capture")
  connect(shiftGate,   "Out", register, "Shift")
  connect(resetGate,   "Out", register, "Reset")

  tie(register, "Offset", offset, "Out")
  tie(register, "Shift",  shift,  "Out")
  tie(register, "Length", length, "Out")
  tie(register, "Stride", stride, "Out")

  tie(register, "Output Gain", outputGain, "Out")
  tie(register, "Output Bias", outputBias, "Out")
  tie(register, "Input Gain",  inputGain,  "Out")
  tie(register, "Input Bias",  inputBias,  "Out")

  for i = 1, channelCount do
    connect(register, "Out", self, "Out"..i)
  end
end

-- function Register:serialize()
--   local t = Unit.serialize(self)

--   t.registers = {}
--   t.registers[1] = self.serializeRegister(self.objects.register)

--   return t
-- end

-- function Register:deserialize(t)
--   Unit.deserialize(self, t)

--   local register = self.objects.register;
--   local rt = t.registers[1];
--   if rt then
--     self.deserializeRegister(register, rt)
--   end
-- end

-- function Register:onShowMenu(objects)
--   return {
--     windowHeader = MenuHeader {
--       description = "Set window (" .. objects.register:getLength() .. ") ..."
--     },
--     zeroWindow = Task {
--       description = "zero",
--       task = function ()
--         objects.register:triggerZeroWindow()
--       end
--     },
--     scatterWindow = Task {
--       description = "scatter",
--       task = function ()
--         objects.register:triggerScatterWindow()
--       end
--     },
--     randomizeWindow = Task {
--       description = "zero + scatter",
--       task = function ()
--         objects.register:triggerRandomizeAll()
--       end
--     },
--     allHeader = MenuHeader {
--       description = "Set all (" .. objects.register:getMax() .. ") ..."
--     },
--     zeroAll = Task {
--       description = "zero",
--       task = function ()
--         objects.register:triggerZeroAll()
--       end
--     },
--     scatterAll = Task {
--       description = "scatter",
--       task = function ()
--         objects.register:triggerScatterAll()
--       end
--     },
--     randomizeAll = Task {
--       description = "zero + scatter",
--       task = function ()
--         objects.register:triggerRandomizeAll()
--       end
--     },
--     clockSync = FlagSelect {
--       description = "Clock Sync",
--       option      = objects.register:getOption("Sync"),
--       flags       = { "shift", "capture", "reset" }
--     }
--   }, {
--     "clockSync",
--     "allHeader",    "zeroAll",    "scatterAll",    "randomizeAll",
--     "windowHeader", "zeroWindow", "scatterWindow", "randomizeWindow"
--   }
-- end

function Register:onLoadViews()
  return {
    length  = GainBias {
      button        = "length",
      description   = "Length",
      branch        = self.branches.length,
      gainbias      = self.objects.length,
      range         = self.objects.length,
      gainMap       = self.intMap(-self.max, self.max),
      biasMap       = self.intMap(1, self.max),
      biasPrecision = 0,
      initialBias   = 16
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
    offset  = GainBias {
      button        = "offset",
      description   = "Offset",
      branch        = self.branches.offset,
      gainbias      = self.objects.offset,
      range         = self.objects.offset,
      gainMap       = self.intMap(-self.max, self.max),
      biasMap       = self.intMap(0, self.max),
      biasPrecision = 0,
      initialBias   = 0
    },
    shift  = GainBias {
      button        = "shift",
      description   = "Shift",
      branch        = self.branches.shift,
      gainbias      = self.objects.shift,
      range         = self.objects.shift,
      gainMap       = self.intMap(-self.max, self.max),
      biasMap       = self.intMap(0, self.max),
      biasPrecision = 0,
      initialBias   = 0
    },
    outputGain   = GainBias {
      button        = "oGain",
      description   = "Output Gain",
      branch        = self.branches.outputGain,
      gainbias      = self.objects.outputGain,
      range         = self.objects.outputGain,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 1
    },
    outputBias   = GainBias {
      button        = "oBias",
      description   = "Output Bias",
      branch        = self.branches.outputBias,
      gainbias      = self.objects.outputBias,
      range         = self.objects.outputBias,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0
    },
    inputGain   = GainBias {
      button        = "iGain",
      description   = "Input Gain",
      branch        = self.branches.inputGain,
      gainbias      = self.objects.inputGain,
      range         = self.objects.inputGain,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 1
    },
    inputBias   = GainBias {
      button        = "iBias",
      description   = "Input Bias",
      branch        = self.branches.inputBias,
      gainbias      = self.objects.inputBias,
      range         = self.objects.inputBias,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0
    },
    register = RegisterView {
      name = "register",
      register = self.objects.register
    }
  }, {
    expanded = { "length", "stride", "offset", "shift", "outputGain", "outputBias", "register" }
  }
  -- return {
  --   wave1 = OutputScope {
  --     monitor = self,
  --     width   = 1 * app.SECTION_PLY
  --   },
  --   wave3 = OutputScope {
  --     monitor = self,
  --     width   = 3 * app.SECTION_PLY
  --   },
  --   clock   = self:gateView("clock", "Advance"),
  --   capture = self:gateView("capture", "Enable Write"),
  --   shift   = self:gateView("shift", "Enable Shift"),
  --   reset   = self:gateView("reset", "Enable Reset"),
  --   length  = GainBias {
  --     button        = "length",
  --     description   = "Length",
  --     branch        = self.branches.length,
  --     gainbias      = self.objects.length,
  --     range         = self.objects.lengthRange,
  --     gainMap       = self.intMap(-self.max, self.max),
  --     biasMap       = self.intMap(1, self.max),
  --     biasPrecision = 0,
  --     initialBias   = 4
  --   },
  --   stride  = GainBias {
  --     button        = "stride",
  --     description   = "Stride",
  --     branch        = self.branches.stride,
  --     gainbias      = self.objects.stride,
  --     range         = self.objects.strideRange,
  --     gainMap       = self.intMap(-self.max / 4, self.max / 4),
  --     biasMap       = self.intMap(-self.max / 4, self.max / 4),
  --     biasPrecision = 0,
  --     initialBias   = 1
  --   },
  --   scatter   = GainBias {
  --     button        = "scatter",
  --     description   = "Scatter",
  --     branch        = self.branches.scatter,
  --     gainbias      = self.objects.scatter,
  --     range         = self.objects.scatter,
  --     biasMap       = Encoder.getMap("[0,1]"),
  --     biasUnits     = app.unitNone,
  --     biasPrecision = 2,
  --     initialBias   = self.initialScatter
  --   },
  --   drift   = GainBias {
  --     button        = "drift",
  --     description   = "Drift",
  --     branch        = self.branches.drift,
  --     gainbias      = self.objects.drift,
  --     range         = self.objects.drift,
  --     biasMap       = Encoder.getMap("[0,1]"),
  --     biasUnits     = app.unitNone,
  --     biasPrecision = 2,
  --     initialBias   = self.initialDrift
  --   },
  --   gain   = GainBias {
  --     button        = "gain",
  --     description   = "Input Gain",
  --     branch        = self.branches.gain,
  --     gainbias      = self.objects.gain,
  --     range         = self.objects.gain,
  --     biasMap       = Encoder.getMap("[0,1]"),
  --     biasUnits     = app.unitNone,
  --     biasPrecision = 2,
  --     initialBias   = self.initialGain
  --   },
  --   bias   = GainBias {
  --     button        = "bias",
  --     description   = "Input Bias",
  --     branch        = self.branches.bias,
  --     gainbias      = self.objects.bias,
  --     range         = self.objects.bias,
  --     biasMap       = Encoder.getMap("[-1,1]"),
  --     biasUnits     = app.unitNone,
  --     biasPrecision = 2,
  --     initialBias   = self.initialBias
  --   }
  -- }, {
  --   expanded  = { "clock", "capture", "length", "stride", "drift" },

  --   clock     = { "clock",   "wave1", "shift", "reset" },
  --   capture   = { "capture", "wave1", "gain", "bias", "scatter" },
  --   length    = { "length",  "wave1", "stride", "drift" },
  --   stride    = { "stride",  "wave3" },
  --   drift     = { "drift",   "wave3" },

  --   collapsed = { "length" }
  -- }
end

return Register
