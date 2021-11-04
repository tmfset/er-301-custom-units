local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local Encoder = require "Encoder"
local GainBias = require "Unit.ViewControl.GainBias"
local Common = require "lojik.Common"

local Curl = Class {}
Curl:include(Unit)
Curl:include(Common)

function Curl:init(args)
  args.title = "Curl"
  args.mnemonic = "C"
  Unit.init(self, args)
end

function Curl:onLoadGraph(channelCount)
  local gain = self:addGainBiasControl("gain")
  local bias = self:addGainBiasControl("bias")
  local fold = self:addGainBiasControl("fold")

  for i = 1, channelCount do
    local op = self:addObject("op", lojik.Curl())
    connect(gain, "Out",   op, "Gain")
    connect(bias, "Out",   op, "Bias")
    connect(fold, "Out",   op, "Fold")
    connect(self, "In"..i, op,   "In")

    connect(op,  "Out",    self, "Out"..i)
  end
end

function Curl:onLoadViews()
  return {
    gain   = GainBias {
      button        = "gain",
      description   = "Input Gain",
      branch        = self.branches.gain,
      gainbias      = self.objects.gain,
      range         = self.objects.gain,
      biasMap       = Encoder.getMap("[-20,20]"),
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
      biasMap       = Encoder.getMap("[-20,20]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0
    },
    fold   = GainBias {
      button        = "fold",
      description   = "Fold",
      branch        = self.branches.fold,
      gainbias      = self.objects.fold,
      range         = self.objects.fold,
      biasMap       = Encoder.getMap("[0,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0
    }
  }, {
    expanded  = { "gain", "bias", "fold" },
    collapsed = {}
  }
end

return Curl
