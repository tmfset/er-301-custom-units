local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local Encoder = require "Encoder"
local Pitch = require "Unit.ViewControl.Pitch"
local GainBias = require "Unit.ViewControl.GainBias"
local UnitShared = require "common.assets.UnitShared"

local Pulse = Class {}
Pulse:include(Unit)
Pulse:include(UnitShared)

function Pulse:init(args)
  args.title = "Pulse"
  args.mnemonic = "P"
  Unit.init(self, args)
end

function Pulse:onLoadGraph(channelCount)
  local tune  = self:addConstantOffsetControl("tune")
  local freq  = self:addGainBiasControl("freq")
  local sync  = self:addComparatorControl("sync", app.COMPARATOR_TRIGGER_ON_RISE)
  local width = self:addGainBiasControl("width")
  local gain  = self:addGainBiasControl("gain")

  local op = self:addObject("op", lojik.Pulse())
  connect(tune,  "Out", op, "V/Oct")
  connect(freq,  "Out", op, "Frequency")
  connect(sync,  "Out", op, "Sync")
  connect(width, "Out", op, "Width")
  connect(gain,  "Out", op, "Gain")

  for i = 1, channelCount do
    connect(op, "Out", self, "Out"..i)
  end
end

function Pulse:onLoadViews()
  return {
    tune = Pitch {
      button      = "V/oct",
      branch      = self.branches.tune,
      description = "V/oct",
      offset      = self.objects.tune,
      range       = self.objects.tuneRange
    },
    freq = GainBias {
      button      = "freq",
      description = "Frequency",
      branch      = self.branches.freq,
      gainbias    = self.objects.freq,
      range       = self.objects.freqRange,
      biasMap     = Encoder.getMap("oscFreq"),
      biasUnits   = app.unitHertz,
      initialBias = 27.5,
      gainMap     = Encoder.getMap("freqGain"),
      scaling     = app.octaveScaling
    },
    sync    = self:gateView("sync", "Sync"),
    width   = GainBias {
      button        = "width",
      description   = "Width",
      branch        = self.branches.width,
      gainbias      = self.objects.width,
      range         = self.objects.widthRange,
      biasMap       = Encoder.getMap("[0,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0.5
    },
    gain   = GainBias {
      button        = "gain",
      description   = "Gain",
      branch        = self.branches.gain,
      gainbias      = self.objects.gain,
      range         = self.objects.gain,
      biasMap       = Encoder.getMap("[-1,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0.5
    }
  }, {
    expanded  = { "tune", "freq", "sync", "width", "gain" },
    collapsed = { }
  }
end

return Pulse
