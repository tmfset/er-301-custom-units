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

local FourStep = Class {}
FourStep:include(Unit)

function FourStep:init(args)
  args.title    = "FourStep"
  args.mnemonic = "fs"

  Unit.init(self, args)
end

function FourStep:createControl(type, name)
  local control = self:createObject(type, name)
  local controlRange = self:createObject("MinMax", name.."Range")
  connect(control, "Out", controlRange, "In")
  self:createMonoBranch(name, control, "In", control, "Out")
  return control
end

function FourStep:createTriggerControl(name)
  local trigger = self:createObject("Comparator", name)
  trigger:setMode(3)
  self:createMonoBranch(name, trigger, "In", trigger, "Out")
  return trigger
end

function FourStep:createControls()
  self._controls = {
    next  = self:createTriggerControl("next"),
    shred = self:createTriggerControl("shred"),
    zero  = self:createTriggerControl("zero"),
    attv  = self:createControl("GainBias", "attv")
  }

  return self._controls
end

function FourStep:onLoadGraph(channelCount)
end

function FourStep:onLoadViews(objects, branches)
  local controls, views = {}, {
    expanded  = { "next", "shred", "zero", "attv" },
    collapsed = { "next" }
  }

  local intMap = function (min, max, precision)
    local map = app.LinearDialMap(min, max)
    map:setSteps(5, 1, 0.25, 0.25)
    map:setRounding(precision)
    return map
  end

  controls.next = Gate {
    button      = "next",
    description = "Proceed",
    branch      = branches.next,
    comparator  = objects.next
  }

  controls.next = Gate {
    button      = "shred",
    description = "Pick Random",
    branch      = branches.shred,
    comparator  = objects.shred
  }

  controls.next = Gate {
    button      = "zero",
    description = "Pick Zero",
    branch      = branches.zero,
    comparator  = objects.zero
  }

  controls.attv = GainBias {
    button        = "attv",
    description   = "Attenuvert",
    branch        = branches.attv,
    gainbias      = objects.attv,
    range         = objects.attvRange,
    gainMap       = intMap(-1, 1, 2),
    biasMap       = intMap(-1, 1, 2),
    biasPrecision = 2,
    initialBias   = config.initialAttv
  }

  return controls, views
end