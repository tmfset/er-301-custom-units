local app = app
local strike = require "strike.libstrike"
local Class = require "Base.Class"
local Encoder = require "Encoder"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Fader = require "Unit.ViewControl.Fader"
local Gate = require "Unit.ViewControl.Gate"
local OutputScope = require "Unit.ViewControl.OutputScope"
local OptionControl = require "Unit.MenuControl.OptionControl"
local Pitch = require "Unit.ViewControl.Pitch"
local SidechainMeter = require "strike.SidechainMeter"
local OutputMeter = require "strike.OutputMeter"

local CPR = Class {}
CPR:include(Unit)

function CPR:init(args)
  args.title = "CPR"
  args.mnemonic = "cpr"
  Unit.init(self, args)
end

function CPR.addComparatorControl(self, name, mode, default)
  local gate = self:addObject(name, app.Comparator())
  gate:setMode(mode)
  self:addMonoBranch(name, gate, "In", gate, "Out")
  if default then
    gate:setOptionValue("State", default)
  end
  return gate
end

function CPR.addGainBiasControlNoBranch(self, name)
  local gb    = self:addObject(name, app.GainBias());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(gb, "Out", range, "In")
  return gb;
end

function CPR.addGainBiasControl(self, name)
  local gb = self:addGainBiasControlNoBranch(name)
  self:addMonoBranch(name, gb, "In", gb, "Out")
  return gb;
end

function CPR.addConstantOffsetControl(self, name)
  local co    = self:addObject(name, app.ConstantOffset())
  local range = self:addObject(name.."Range", app.MinMax())
  connect(co, "Out", range, "In")
  self:addMonoBranch(name, co, "In", co, "Out")
  return co;
end

function CPR.linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

function CPR:onLoadGraph(channelCount)
  local op = self:addObject("op", strike.CPR())

  for i = 1, channelCount do
    connect(self, "In"..i, op, "In"..i)
    connect(op,   "Out"..i, self, "Out"..i)
  end

  local monitor = self:addObject("monitor", app.Monitor());
  connect(monitor, "Out", op, "Sidechain")
  self:addMonoBranch("sidechain", monitor, "In", monitor, "Out")
  -- self:addMonoBranch("ratio", ratio, "In", op, "Reduction")
  -- self:addMonoBranch("rise", rise, "In", op, "EOF")
  -- self:addMonoBranch("fall", fall, "In", op, "EOR")
end

function CPR.defaultDecibelMap()
  local map = app.LinearDialMap(-60, 12)
  map:setZero(0)
  map:setSteps(6, 1, 0.1, 0.01);
  return map
end

function CPR:onLoadViews()
  return {
    input = SidechainMeter {
      button       = "input",
      description  = "Input Gain",
      branch       = self.branches.sidechain,
      compressor   = self.objects.op,
      channelCount = self.channelCount,
      map          = self.defaultDecibelMap(),
      units        = app.unitDecibels,
      scaling      = app.linearScaling
    },
    rise = Fader {
      button      = "rise",
      description = "Rise Time",
      param       = self.objects.op:getParameter("Rise"),
      map         = self.linMap(0, 10, 0.1, 0.01, 0.001, 0.001),
      units       = app.unitSecs
    },
    fall = Fader {
      button      = "fall",
      description = "Fall Time",
      param       = self.objects.op:getParameter("Fall"),
      map         = self.linMap(0, 10, 0.1, 0.01, 0.001, 0.001),
      units       = app.unitSecs
    },
    threshold = Fader {
      button        = "thresh",
      description   = "Threshold",
      param         = self.objects.op:getParameter("Threshold"),
      map           = self.defaultDecibelMap(),
      units         = app.unitDecibels
    },
    ratio = Fader {
      button       = "ratio",
      description  = "Ratio",
      param        = self.objects.op:getParameter("Ratio"),
      map          = Encoder.getMap("[0,10]")
    },
    output = OutputMeter {
      button       = "output",
      description  = "Output Gain",
      compressor   = self.objects.op,
      channelCount = self.channelCount,
      map          = self.defaultDecibelMap(),
      units        = app.unitDecibels,
      scaling      = app.linearScaling
    }
  }, {
    expanded  = { "input", "output", "threshold", "ratio", "rise", "fall" },
    collapsed = {}
  }
end

return CPR
