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
local Common = require "strike.Common"

local CPR = Class {}
CPR:include(Unit)
CPR:include(Common)

function CPR:init(args)
  args.title = "CPR"
  args.mnemonic = "cpr"
  Unit.init(self, args)
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
  self:addFreeBranch("active", op, "Active")
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
    sidechain = self:branchControlView("sidechain"),
    active    = self:branchControlView("active"),
    reduction = self:branchControlView("reduction"),
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
    scope = { "sidechain", "threshold", "reduction", "active" },
    expanded  = { "input", "threshold", "scope", "output" },
    collapsed = { "scope" }
  }
end

return CPR
