local app = app
local Gate = require "Unit.ViewControl.Gate"
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

function Common.addParameterAdapterControl(self, name)
  local pa = self:addObject(name, app.ParameterAdapter())
  self:addMonoBranch(name, pa, "In", pa, "Out")
  return pa
end

-- Create a memoized constant value to be used as an output.
function Common.mConst(self, value)
  self.constants = self.constants or {}

  if self.constants[value] == nil then
    local const = self:addObject("constant"..value, app.Constant())
    const:hardSet("Value", value)

    self.constants[value] = const
  end

  return self.constants[value]
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

function Common.pick(self, pick, left, right, name)
  local op = self:addObject(name or ("Pick"..left:name()..right:name()), lojik.Pick())
  connect(left,  "Out", op, "Left")
  connect(right, "Out", op, "Right")
  connect(pick,  "Out", op, "Pick")
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

function Common.intMap(min, max)
  local map = app.LinearDialMap(min,max)
    map:setSteps(1, 1, 1, 1)
    map:setRounding(1)
    return map
end

function Common.gateView(self, name, description)
  return Gate {
    button      = name,
    description = description,
    branch      = self.branches[name],
    comparator  = self.objects[name]
  }
end

function Common.serializeRegister(register)
  local max = register:getMax()
  local data = {}

  for i = 1, max do
    data[i] = register:getData(i - 1);
  end

  return {
    max    = max,
    step   = register:getStep(),
    shift  = register:getShift(),
    length = register:getSeqLength(),
    data   = data
  }
end

function Common.deserializeRegister(register, t)
  register:setMax(t.max)
  register:setStep(t.step)
  register:setShift(t.shift)
  register:setSeqLength(t.length)

  for i, v in ipairs(t.data) do
    register:setData(i - 1, v)
  end

  register:triggerDeserialize();
end

return Common
