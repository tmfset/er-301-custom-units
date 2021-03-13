local app = app
local lojik = require "lojik.liblojik"

local Common = {}

function Common.addComparatorControl(self, name, mode, default)
  local gate = self:addObject(name, app.Comparator())
  gate:setMode(mode)
  self:addMonoBranch(name, gate, "In", gate, "Out")
  if default then
    gate:setOptionValue("State", default)
  end
  return gate
end

function Common.addGainBiasControl(self, name)
  local gb    = self:addObject(name, app.GainBias());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(gb, "Out", range, "In")
  self:addMonoBranch(name, gb, "In", gb, "Out")
  return gb;
end

function Common.lAnd(self, left, right, name, lOut, rOut)
  local op = self:addObject(name or (left:name().."And"..right:name()), lojik.And())
  connect(left,  lOut or "Out", op, "Left")
  connect(right, rOut or "Out", op, "Right")
  return op
end

function Common.lOr(self, left, right, name, lOut, rOut)
  local op = self:addObject(name or (left:name().."Or"..right:name()), lojik.Or())
  connect(left,  lOut or "Out", op, "Left")
  connect(right, rOut or "Out", op, "Right")
  return op
end

function Common.lNot(self, input, name, iOut)
  local op = self:addObject(name or ("Not"..input:name()), lojik.Not())
  connect(input, iOut or "Out", op, "In")
  return op
end

function Common.lNotTrig(self, input)
  return self:trig(self:lNot(input))
end

function Common.trig(self, input, name, iOut)
  local op = self:addObject(name or ("Trig"..input:name()), lojik.Trig())
  connect(input, iOut or "Out", op, "In")
  return op
end

function Common.latch(self, set, reset, name, sOut, rOut)
  local op = self:addObject(name or ("Latch"..set:name()..reset:name()), lojik.Latch())
  connect(set,   sOut or "Out", op, "Set")
  connect(reset, rOut or "Out", op, "Reset")
  return op
end

function Common.dLatch(self, input, clock, reset, name, iName, cName, rName)
  local op = self:addObject(name or ("DLatch"..input:name()), lojik.DLatch())
  connect(input, iName or "Out", op, "In")
  connect(clock, cName or "Out", op, "Clock")
  connect(reset, rName or "Out", op, "Reset")
  return op
end

return Common
