local app = app
local strike = require "strike.libstrike"
local Class = require "Base.Class"
local GainBias = require "Unit.ViewControl.GainBias"
local Unit = require "Unit"
local Common = require "strike.Common"

local Tanh = Class {}
Tanh:include(Unit)
Tanh:include(Common)

function Tanh:init(args)
  args.title = "Tanh"
  args.mnemonic = "tanh"
  Unit.init(self, args)
end

function Tanh:onLoadGraph(channelCount)
  local gain = self:addGainBiasControl("gain")

  for i = 1, channelCount do
    local op = self:addObject("op"..i, strike.Tanh())
    connect(self, "In"..i, op, "In")
    connect(gain, "Out", op, "Gain")
    connect(op, "Out", self, "Out"..i)
  end
end

function Tanh:onLoadViews()
  return {
    gain = GainBias {
      button        = "gain",
      description   = "Input Gain",
      branch        = self.branches.gain,
      gainbias      = self.objects.gain,
      range         = self.objects.gainRange,
      biasMap       = self.linMap(  0, 10, 0.1, 0.01, 0.001, 0.001),
      gainMap       = self.linMap(-10, 10, 0.1, 0.01, 0.001, 0.001),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 2
    }
  }, {
    expanded  = { "gain" },
    collapsed = {}
  }
end

return Tanh
