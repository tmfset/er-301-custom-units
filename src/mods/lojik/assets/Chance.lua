local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Encoder = require "Encoder"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local OptionControl = require "Unit.MenuControl.OptionControl"
local Common = require "lojik.Common"

local Chance = Class {}
Chance:include(Unit)
Chance:include(Common)

function Chance:init(args)
  args.title = "Chance"
  args.mnemonic = "?"
  Unit.init(self, args)
end

function Chance:onLoadGraph(channelCount)
  local chance = self:addGainBiasControl("chance")

  local op = self:addObject("op", lojik.Chance())
  connect(self,   "In1", op, "In")
  connect(chance, "Out", op, "Chance")

  for i = 1, channelCount do
    connect(op, "Out", self, "Out"..i)
  end
end

function Chance:onShowMenu(objects)
  return {
    mode = OptionControl {
      description = "Output Mode",
      option      = objects.op:getOption("Mode"),
      choices     = { "trigger", "gate", "through" }
    },
    sensitivity = self.senseOptionControl(objects.op)
  }, { "mode", "sensitivity" }
end

function Chance:onLoadViews()
  return {
    chance = GainBias {
      button        = "chance",
      description   = "Chance",
      branch        = self.branches.chance,
      gainbias      = self.objects.chance,
      range         = self.objects.chanceRange,
      biasMap       = Encoder.getMap("[0,1]"),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 0.5
    }
  }, {
    expanded  = { "chance" },
    collapsed = {}
  }
end

return Chance
