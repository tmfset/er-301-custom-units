local app = app
local core = require "core.libcore"
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local Pitch = require "Unit.ViewControl.Pitch"
local Common = require "lojik.Common"

local Step = Class {}
Step:include(Unit)
Step:include(Common)

function Step:init(args)
  args.title = "Step"
  args.mnemonic = "S"
  self.max = 128
  Unit.init(self, args)
end

function Step:onLoadGraph(channelCount)
  local clock   = self:addComparatorControl("clock",  app.COMPARATOR_TRIGGER_ON_RISE)
  local record  = self:addComparatorControl("record", app.COMPARATOR_TOGGLE)
  local pitch   = self:addGainBiasControl("pitch")
  local gate    = self:addComparatorControl("gate",   app.COMPARATOR_GATE)
  local rest    = self:addComparatorControl("rest",   app.COMPARATOR_GATE)

  local cAdvance = self:lAnd(clock, self:lNot(record))
  local rAdvance = self:lNotTrig(self:lOr(gate, rest))
  local advance  = self:lOr(cAdvance, rAdvance)

  local reset = self:trig(record)

  local counter = self:addObject("counter", core.Counter())
  counter:hardSet("Start", 1)
  counter:hardSet("Finish", self.max)
  counter:hardSet("Step Size", 1)
  counter:setOptionValue("Wrap", app.CHOICE_NO)
  connect(rAdvance, "Out", counter, "In")
  connect(reset,    "Out", counter, "Reset")

  for i = 1, channelCount do
    if i % 2 == 1 then
      local pRegister = self:addObject("pRegister", lojik.Register(self.max))
      connect(pitch,   "Out", pRegister, "In")
      connect(counter, "Out", pRegister, "Length")
      connect(record,  "Out", pRegister, "Capture")
      connect(advance, "Out", pRegister, "Clock")
      connect(reset,   "Out", pRegister, "Reset")

      connect(pRegister, "Out", self, "Out"..i)
    end

    if i % 2 == 0 then
      local gRegister = self:addObject("gRegister", lojik.Register(self.max))
      connect(gate,    "Out", gRegister, "In")
      connect(counter, "Out", gRegister, "Length")
      connect(record,  "Out", gRegister, "Capture")
      connect(advance, "Out", gRegister, "Clock")
      connect(reset,   "Out", gRegister, "Reset")

      connect(gRegister, "Out", self, "Out"..i)
    end
  end
end

function Step:onLoadViews()
  return {
    clock  = self:gateView("clock", "Clock"),
    record = self:gateView("record", "Record"),
    gate   = self:gateView("gate", "Gate"),
    rest   = self:gateView("rest", "Rest"),
    pitch = Pitch {
      button      = "V/oct",
      branch      = self.branches.pitch,
      description = "V/oct",
      offset      = self.objects.pitch,
      range       = self.objects.pitchRange
    }
  }, {
    expanded  = { "clock", "record", "pitch", "gate", "rest" },
    collapsed = { "clock", "record" }
  }
end

return Step
