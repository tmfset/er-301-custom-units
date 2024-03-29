local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local UnitShared = require "common.assets.UnitShared"

local Wait = Class {}
Wait:include(Unit)
Wait:include(UnitShared)

function Wait:init(args)
  args.title = "Wait"
  args.mnemonic = "W"
  self.max = 64
  Unit.init(self, args)
end

function Wait:onLoadGraph(channelCount)
  local count  = self:addGainBiasControl("count")
  local invert = self:addComparatorControl("invert", app.COMPARATOR_TOGGLE)
  local arm    = self:addComparatorControl("arm", app.COMPARATOR_TOGGLE)

  for i = 1, channelCount do
    local op = self:addObject("op"..i, lojik.Wait())
    connect(self, "In"..i, op, "In")

    connect(count,  "Out", op, "Count")
    connect(invert, "Out", op, "Invert")
    connect(arm,    "Out", op, "Arm")

    if i > 1 then
      tie(op, "Input Sense", self.objects.op1, "Input Sense")
    end

    connect(op, "Out", self, "Out"..i)
  end
end


function Wait:onShowMenu(objects)
  return {
    sensitivity = self.senseOptionControl(objects.op1)
  }, { "mode", "sensitivity" }
end

function Wait:onLoadViews()
  return {
    count  = GainBias {
      button        = "count",
      description   = "Count",
      branch        = self.branches.count,
      gainbias      = self.objects.count,
      range         = self.objects.countRange,
      gainMap       = self.intMap(-self.max, self.max),
      biasMap       = self.intMap(0, self.max),
      biasPrecision = 0,
      initialBias   = 4
    },
    invert  = self:gateView("invert", "Invert"),
    arm = self:gateView("arm", "Arm")
  }, {
    expanded  = { "count", "invert", "arm" },
    collapsed = {}
  }
end

return Wait
