local app = app
local Gate = require "Unit.ViewControl.Gate"
local lojik = require "lojik.liblojik"
local OptionControl = require "Unit.MenuControl.OptionControl"

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

function Common.addConstantOffsetControl(self, name)
  local co    = self:addObject(name, app.ConstantOffset())
  local range = self:addObject(name.."Range", app.MinMax())
  connect(co, "Out", range, "In")
  self:addMonoBranch(name, co, "In", co, "Out")
  return co
end

function Common.addParameterAdapterControl(self, name)
  local pa = self:addObject(name, app.ParameterAdapter())
  self:addMonoBranch(name, pa, "In", pa, "Out")
  return pa
end

function Common.senseOptionControl(op)
  return OptionControl {
    description = "Input Sense",
    option      = op:getOption("Sense"),
    choices     = { "low", "high" }
  }
end

function Common.mult(self, left, right, name)
  local op = self:addObject(name or ("Mult"..left:name()..right:name()), app.Multiply())
  connect(left,  "Out", op, "Left")
  connect(right, "Out", op, "Right")
  return op
end

function Common.intMap(min, max)
  local map = app.LinearDialMap(min,max)
    map:setSteps(2, 1, 0.25, 0.25)
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
