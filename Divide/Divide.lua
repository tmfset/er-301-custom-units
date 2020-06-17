-- luacheck: globals app connect
local app = app
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"
local InputGate = require "Unit.ViewControl.InputGate"
local OutputScope = require "Unit.ViewControl.OutputScope"
local ply = app.SECTION_PLY

local Divide = Class {}
Divide:include(Unit)

function Divide:init(args)
  args.title    = "Divide"
  args.mnemonic = "c/"
  self.max      = args.max or 64
  self.initial  = args.initial or 4

  Unit.init(self, args)
end

-- Create a memoized constant value to be used as an output.
function Divide:mConst(value)
  self.constants = self.constants or {}

  if self.constants[value] == nil then
    local const = self:createObject("Constant", "constant"..value)
    const:hardSet("Value", value)

    self.constants[value] = const
  end

  return self.constants[value]
end

function Divide:cGainBias(gain, bias, name)
  local gb = self:createObject("GainBias", name)
  gb:hardSet("Gain", gain)
  gb:hardSet("Bias", bias)
  return gb
end

function Divide:toGate(input, name)
  local gate = self:createObject("Comparator", name)
  gate:setGateMode()
  connect(input, "Out", gate, "In")
  return gate
end

function Divide:mult(left, right, name)
  local mult = self:createObject("Multiply", name)
  connect(left, "Out", mult, "Left")
  connect(right, "Out", mult, "Right")
  return mult
end

function Divide:sum(left, right, name)
  local sum = self:createObject("Sum", name)
  connect(left, "Out", sum, "Left")
  connect(right, "Out", sum, "Right")
  return sum
end

function Divide:logicalNot(input, name)
  local gb = self:cGainBias(-1, 1, name)
  connect(input, "Out", gb, "In")
  return gb
end

function Divide:vFinishCounter(start, finish, stepSize, gain, suffix)
  local name = function (str) return "vFinishCounter"..str..suffix end

  local counter = self:createObject("Counter", name("Count"))
  counter:setOptionValue("Processing Rate", 2) -- Sample Rate
  counter:hardSet("Start", start)
  counter:hardSet("Step Size", stepSize)
  counter:hardSet("Gain", gain)
  counter:hardSet("Value", start)

  local finishAdapter = self:createObject("ParameterAdapter", name("Finish"))
  finishAdapter:hardSet("Gain", 1)
  connect(finish, "Out", finishAdapter, "In")
  tie(counter, "Finish", finishAdapter, "Out")

  return counter
end

function Divide:controls()
  local clock = self:createObject("Comparator", "clock")
  clock:setTriggerMode()
  connect(self, "In1", clock, "In")

  local reset = self:createObject("Comparator", "reset")
  reset:setTriggerMode()
  self:createMonoBranch("reset", reset, "In", reset,  "Out")

  local divide = self:createObject("GainBias", "divide")
  local divideRange = self:createObject("MinMax", "divideRange")
  connect(divide, "Out", divideRange, "In")
  self:createMonoBranch("divide", divide, "In", divide, "Out")

  return {
    clock  = clock,
    reset  = reset,
    divide = divide
  }
end

function Divide:onLoadGraph(channelCount)
  local controls = self:controls()

  local steps   = self:sum(controls.divide, self:mConst(-1), "steps")
  local counter = self:vFinishCounter(0, steps, 1, 1, "counter")
  connect(controls.clock, "Out", counter, "In")
  connect(controls.reset, "Out", counter, "Reset")

  local gate   = self:toGate(counter, "gate")
  local output = self:mult(controls.clock, gate, "output")
  connect(output, "Out", self, "Out1")
end

function Divide:onLoadViews(objects, branches)
  local controls, views = {}, {
    expanded  = { "clock", "reset", "divide" },
    collapsed = { "scope2" },

    clock     = { "scope2", "clock" },
    reset     = { "scope2", "reset" },
    divide    = { "scope2", "divide" }
  }

  controls.scope2 = OutputScope {
    monitor = self,
    width   = 2 * ply,
  }

  controls.clock = InputGate {
    button      = "clock",
    description = "Unit Input",
    comparator  = objects.clock,
  }

  controls.reset = Gate {
    button      = "reset",
    description = "Reset",
    branch      = branches.reset,
    comparator  = objects.reset
  }

  local intMap = function (min, max)
    local map = app.LinearDialMap(min,max)
    map:setSteps(5, 1, 0.25, 0.25)
    map:setRounding(1)
    return map
  end

  controls.divide = GainBias {
    button        = "divide",
    description   = "Divide By",
    branch        = branches.divide,
    gainBias      = objects.divide,
    range         = objects.divideRange,
    gainMap       = intMap(-self.max, self.max),
    biasMap       = intMap(1, self.max),
    biasPrecision = 0,
    initialBias   = self.initial
  }

  return controls, views
end

return Divide
