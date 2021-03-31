local app = app
local lojik = require "lojik.liblojik"
local EuclidCircle = require "lojik.EuclidCircle"
local Class = require "Base.Class"
local Encoder = require "Encoder"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local FlagSelect = require "Unit.MenuControl.FlagSelect"
local MenuHeader = require "Unit.MenuControl.Header"
local OptionControl = require "Unit.MenuControl.OptionControl"
local Common = require "lojik.Common"

local Euclid = Class {}
Euclid:include(Unit)
Euclid:include(Common)

function Euclid:init(args)
  args.title = "Euclid"
  args.mnemonic = "E"
  self.max = 32
  Unit.init(self, args)
end

function Euclid:onLoadGraph(channelCount)
  local reset  = self:addComparatorControl("reset", app.COMPARATOR_TRIGGER_ON_RISE)
  local rotate = self:addComparatorControl("rotate", app.COMPARATOR_TRIGGER_ON_RISE)
  local beats  = self:addParameterAdapterControl("beats")
  local length = self:addParameterAdapterControl("length")

  local op = self:addObject("op", lojik.Euclid(self.max))
  connect(self,   "In1", op, "Clock")
  connect(reset,  "Out", op, "Reset")
  connect(rotate, "Out", op, "Rotate")

  tie(op, "Beats",  beats,  "Out")
  tie(op, "Length", length, "Out")

  for i = 1, channelCount do
    connect(op, "Out", self, "Out"..i)
  end
end

function Euclid:onShowMenu(objects)
  return {
    mode = OptionControl {
      description = "Output Mode",
      option      = objects.op:getOption("Mode"),
      choices     = { "trigger", "gate", "through" }
    },
    clockSync = FlagSelect {
      description = "Clock Sync",
      option      = objects.op:getOption("Sync"),
      flags       = { "rotate", "reset" }
    }
  }, { "mode", "clockSync" }
end

function Euclid:onLoadViews()
  return {
    reset   = self:gateView("reset", "Reset"),
    rotate  = self:gateView("rotate", "Rotate"),
    beats   = GainBias {
      button        = "beats",
      description   = "Beats",
      branch        = self.branches.beats,
      gainbias      = self.objects.beats,
      range         = self.objects.beats,
      biasMap       = self.intMap(0, self.max),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 4
    },
    length   = GainBias {
      button        = "length",
      description   = "Length",
      branch        = self.branches.length,
      gainbias      = self.objects.length,
      range         = self.objects.length,
      biasMap       = self.intMap(1, self.max),
      biasUnits     = app.unitNone,
      biasPrecision = 2,
      initialBias   = 7
    },
    circle = EuclidCircle {
      description = "Euclidean Rythm",
      width       = 2 * app.SECTION_PLY,
      euclid      = self.objects.op,
      max         = self.max,
      beats       = self.objects.beats:getParameter("Bias"),
      length      = self.objects.length:getParameter("Bias")
    }
  }, {
    expanded  = { "circle" },

    circle = { "circle", "reset", "rotate", "beats", "length" },

    collapsed = { "circle" }
  }
end

return Euclid
