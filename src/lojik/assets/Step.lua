local app = app
local core = require "core.libcore"
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local Pitch = require "Unit.ViewControl.Pitch"
local GainBias = require "Unit.ViewControl.GainBias"
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
  local stride  = self:addGainBiasControl("stride")
  local gate    = self:addComparatorControl("gate",   app.COMPARATOR_GATE)
  local rest    = self:addComparatorControl("rest",   app.COMPARATOR_GATE)

  local cAdvance = self:lAnd(clock, self:lNot(record))
  local rAdvance = self:lOr(self:lNotTrig(gate), self:lNotTrig(rest))
  local advance  = self:lOr(cAdvance, gate)

  local reset = self:trig(record)

  local counter = self:addObject("counter", core.Counter())
  counter:hardSet("Start", 0)
  counter:hardSet("Finish", self.max)
  counter:hardSet("Step Size", 1)
  counter:setOptionValue("Wrap", app.CHOICE_NO)
  connect(rAdvance, "Out", counter, "In")
  connect(reset,    "Out", counter, "Reset")

  local length = self:pick(record, self:mConst(self.max), counter)

  for i = 1, channelCount do
    if i % 2 == 1 then
      local pRegister = self:addObject("register"..i, lojik.Register(self.max, false))
      connect(pitch,   "Out", pRegister, "In")

      local one = self:mConst(1)
      connect(advance, "Out", pRegister, "Clock")
      connect(record,  "Out", pRegister, "Capture")
      connect(length,  "Out", pRegister, "Length")
      connect(stride,  "Out", pRegister, "Stride")
      connect(reset,   "Out", pRegister, "Reset")
      connect(one,     "Out", pRegister, "Gain")

      connect(pRegister, "Out", self, "Out"..i)
    end

    if i % 2 == 0 then
      local gRegister = self:addObject("register"..i, lojik.Register(self.max, false))
      connect(gate,    "Out", gRegister, "In")

      local one = self:mConst(1)
      connect(advance, "Out", gRegister, "Clock")
      connect(record,  "Out", gRegister, "Capture")
      connect(length,  "Out", gRegister, "Length")
      connect(stride,  "Out", gRegister, "Stride")
      connect(reset,   "Out", gRegister, "Reset")
      connect(one,     "Out", gRegister, "Gain")

      connect(gRegister, "Out", self, "Out"..i)
    end
  end
end

function Step:serialize()
  local t = Unit.serialize(self)
  t.registers = {}

  for i = 1, self.channelCount do
    local register = self.objects["register"..i]
    if register then
      t.registers[i] = self.serializeRegister(register)
    end
  end

  return t
end

function Step:deserialize(t)
  Unit.deserialize(self, t)

  for i, rt in ipairs(t.registers or {}) do
    local register = self.objects["register"..i]
    if register then
      self.deserializeRegister(register, rt)
    end
  end
end

function Step:onLoadViews()
  return {
    clock  = self:gateView("clock", "Clock"),
    record = self:gateView("record", "Record"),
    gate   = self:gateView("gate", "Gate"),
    rest   = self:gateView("rest", "Rest"),
    stride  = GainBias {
      button        = "stride",
      description   = "Stride",
      branch        = self.branches.stride,
      gainbias      = self.objects.stride,
      range         = self.objects.strideRange,
      gainMap       = self.intMap(-self.max, self.max),
      biasMap       = self.intMap(1, self.max),
      biasPrecision = 0,
      initialBias   = 1
    },
    pitch = Pitch {
      button      = "V/oct",
      branch      = self.branches.pitch,
      description = "V/oct",
      offset      = self.objects.pitch,
      range       = self.objects.pitchRange
    }
  }, {
    expanded  = { "clock", "record", "pitch", "gate", "rest" },
    clock     = { "clock", "stride" },
    collapsed = { "clock", "record" }
  }
end

return Step
