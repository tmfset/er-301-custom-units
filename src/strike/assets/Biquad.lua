local app = app
local strike = require "strike.libstrike"
local Class = require "Base.Class"
local Encoder = require "Encoder"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"

local Biquad = Class {}
Biquad:include(Unit)

function Biquad:init(args)
  args.title = "Biquad"
  args.mnemonic = "B"
  Unit.init(self, args)
end

function Biquad.addGainBiasControl(self, name)
    local gb    = self:addObject(name, app.GainBias());
    local range = self:addObject(name.."Range", app.MinMax())
    connect(gb, "Out", range, "In")
    self:addMonoBranch(name, gb, "In", gb, "Out")
    return gb;
  end

function Biquad:onLoadGraph(channelCount)
  local value = self:addGainBiasControl("value")
  local q     = self:addGainBiasControl("q")

  local op = self:addObject("op", strike.Biquad())
  connect(self, "In1", op, "In")
  connect(value, "Out", op, "Value")
  connect(q,     "Out", op, "Q")

  for i = 1, channelCount do
    connect(op, "Out", self, "Out"..i)
  end
end

function Biquad:onLoadViews()
  return {
    value   = GainBias {
      button        = "value",
      description   = "Something",
      branch        = self.branches.value,
      gainbias      = self.objects.value,
      range         = self.objects.valueRange,
      biasMap       = Encoder.getMap("[0,10]"),
      biasUnits     = app.unitNone,
      biasPrecision = 3,
      initialBias   = 1
    },
    q   = GainBias {
      button        = "q",
      description   = "Q",
      branch        = self.branches.q,
      gainbias      = self.objects.q,
      range         = self.objects.qRange,
      biasMap       = Encoder.getMap("[0,10]"),
      biasUnits     = app.unitNone,
      biasPrecision = 3,
      initialBias   = 0.5
    }
  }, {
    expanded  = { "value", "q" },
    collapsed = {}
  }
end

return Biquad
