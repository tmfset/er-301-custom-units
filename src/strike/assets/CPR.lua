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
local CompressorScope = require "strike.CompressorScope"
local BranchControl = require "strike.BranchControl"

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

function CPR.addParameterAdapterControl(self, name)
  local pa = self:addObject(name, app.ParameterAdapter())
  self:addMonoBranch(name, pa, "In", pa, "Out")
  return pa
end

function CPR.addFreeBranch(self, name, obj, outlet)
  local monitor = self:addObject(name.."Monitor", app.Monitor())
  self:addMonoBranch(name, monitor, "In", obj, outlet)
end

function CPR.addMonitorBranch(self, name, obj, outlet)
  local monitor = self:addObject(name.."Monitor", app.Monitor())
  connect(monitor, "Out", obj, outlet)
  self:addMonoBranch(name, monitor, "In", monitor, "Out")
end

function CPR.linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

function CPR:onLoadGraph(channelCount)
  local op = self:addObject("op", strike.CPR())

  local threshold = self:addParameterAdapterControl("threshold")
  tie(op, "Threshold", threshold, "Out")

  for i = 1, channelCount do
    connect(self, "In"..i, op, "In"..i)
    connect(op,   "Out"..i, self, "Out"..i)
  end

  self:addMonitorBranch("sidechain", op, "Sidechain")
  self:addFreeBranch("reduction", op, "Reduction")
  self:addFreeBranch("eof", op, "EOF")
  self:addFreeBranch("eor", op, "EOR")
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
    scope = CompressorScope {
      description  = "Compression",
      width        = app.SECTION_PLY * 2,
      compressor   = self.objects.op
    },
    threshold = GainBias {
      button        = "thresh",
      description   = "Threshold",
      branch        = self.branches.threshold,
      gainbias      = self.objects.threshold,
      range         = self.objects.threshold,
      biasMap       = Encoder.getMap("[0,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0.5
    },
    sidechain = BranchControl {
      name = "sidechain",
      branch = self.branches.sidechain
    },
    eof = BranchControl {
      name = "eof",
      branch = self.branches.eof
    },
    eor = BranchControl {
      name = "eor",
      branch = self.branches.eor
    },
    reduction = BranchControl {
      name = "reduction",
      branch = self.branches.reduction
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
    scope = { "sidechain", "threshold", "reduction", "eof", "eor" },
    expanded  = { "input", "threshold", "scope", "output" },
    collapsed = { "scope" }
  }
end

return CPR
