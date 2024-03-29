local app = app
local Gate = require "Unit.ViewControl.Gate"
local Branch = require "common.assets.ViewControl.Branch"
local OptionControl = require "Unit.MenuControl.OptionControl"

local UnitControls = {}

function UnitControls.addComparatorControl(self, name, mode, default)
  local gate = self:addObject(name, app.Comparator())
  gate:setMode(mode)
  self:addMonoBranch(name, gate, "In", gate, "Out")
  if default then
    gate:setOptionValue("State", default)
  end
  return gate
end

function UnitControls.addGainBiasControl(self, name)
  local gb    = self:addObject(name, app.GainBias());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(gb, "Out", range, "In")
  self:addMonoBranch(name, gb, "In", gb, "Out")
  return gb;
end

function UnitControls.addConstantOffsetControl(self, name)
  local co    = self:addObject(name, app.ConstantOffset());
  local range = self:addObject(name.."Range", app.MinMax())
  connect(co, "Out", range, "In")
  self:addMonoBranch(name, co, "In", co, "Out")
  return co
end

function UnitControls.addParameterAdapterControl(self, name)
  local pa = self:addObject(name, app.ParameterAdapter())
  self:addMonoBranch(name, pa, "In", pa, "Out")
  return pa
end

function UnitControls.addFreeBranch(self, name, obj, outlet)
  local monitor = self:addObject(name.."Monitor", app.Monitor())
  self:addMonoBranch(name, monitor, "In", obj, outlet)
end

function UnitControls.addMonitorBranch(self, name, obj, outlet)
  local monitor = self:addObject(name.."Monitor", app.Monitor())
  connect(monitor, "Out", obj, outlet)
  self:addMonoBranch(name, monitor, "In", monitor, "Out")
end

function UnitControls.intMap(min, max)
  local map = app.LinearDialMap(min,max)
    map:setSteps(2, 1, 0.25, 0.25)
    map:setRounding(1)
    return map
end

function UnitControls.linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

function UnitControls.defaultDecibelMap()
  local map = app.LinearDialMap(-60, 12)
  map:setZero(0)
  map:setSteps(6, 1, 0.1, 0.01);
  return map
end

function UnitControls.senseOptionControl(op)
  return OptionControl {
    description = "Input Sense",
    option      = op:getOption("Sense"),
    choices     = { "low", "high" }
  }
end

function UnitControls.gateView(self, name, description)
  return Gate {
    button      = name,
    description = description,
    branch      = self.branches[name],
    comparator  = self.objects[name]
  }
end

function UnitControls.branchView(self, name)
  return Branch {
    name   = name,
    branch = self.branches[name]
  }
end

return UnitControls
