-- luacheck: globals app connect
local app = app
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"
local Encoder = require "Encoder"
local SamplePool = require "Sample.Pool"
local RecordingView = require "builtins.Looper.RecordingView"
local SamplePoolInterface = require "Sample.Pool.Interface"
local OptionControl = require "Unit.MenuControl.OptionControl"
local Task = require "Unit.MenuControl.Task"
local MenuHeader = require "Unit.MenuControl.Header"
local ply = app.SECTION_PLY

local config = require "Strike.defaults"

local Wait = Class {}
Wait:include(Unit)

function Wait:init(args)
  args.title    = "Wait"
  args.mnemonic = "wt"

  Unit.init(self, args)
end

function Wait:createControl(type, name)
  local control = self:createObject(type, name)
  local controlRange = self:createObject("MinMax", name.."Range")
  connect(control, "Out", controlRange, "In")
  self:createMonoBranch(name, control, "In", control, "Out")
  return control
end

function Wait:createGateControl(name)
  local trigger = self:createObject("Comparator", name)
  trigger:setMode(2)
  self:createMonoBranch(name, trigger, "In", trigger, "Out")
  return trigger
end

function Wait:createControls()
  self._controls = {
    wait   = self:createGateControl("wait"),
    steps  = self:createControl("GainBias", "steps")
  }

  return self._controls
end

function Wait:onLoadGraph(channelCount)
end

function Wait:onLoadViews(objects, branches)
  local controls, views = {}, {
    expanded  = { "wait", "steps" },
    collapsed = { "wait" }
  }

  local intMap = function (min, max)
    local map = app.LinearDialMap(min, max)
    map:setSteps(5, 1, 0.25, 0.25)
    map:setRounding(1)
    return map
  end

  controls.wait = Gate {
    button      = "wait",
    description = "Hold Please!",
    branch      = branches.strike,
    comparator  = objects.strike
  }

  controls.steps = GainBias {
    button        = "steps",
    description   = "How Long?",
    branch        = branches.steps,
    gainbias      = objects.steps,
    range         = objects.stepsRange,
    gainMap       = intMap(-config.maxSteps, config.maxSteps),
    biasMap       = intMap(1, config.maxSteps),
    biasPrecision = 0,
    initialBias   = config.initialSteps
  }

  return controls, views
end