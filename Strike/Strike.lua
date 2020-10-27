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

local Strike = Class {}
Strike:include(Unit)

function Strike:init(args)
  args.title    = "Strike"
  args.mnemonic = "lpg"

  Unit.init(self, args)
end

function Strike:createControl(type, name)
  local control = self:createObject(type, name)
  local controlRange = self:createObject("MinMax", name.."Range")
  connect(control, "Out", controlRange, "In")
  self:createMonoBranch(name, control, "In", control, "Out")
  return control
end

function Strike:createTriggerControl(name)
  local trigger = self:createObject("Comparator", name)
  trigger:setMode(3)
  self:createMonoBranch(name, trigger, "In", trigger, "Out")
  return trigger
end

function Strike:createControls()
  self._controls = {
    strike = self:createTrigger("strike"),
    lift   = self:createControl("GainBias", "lift"),
    Q      = self:createControl("GainBias", "Q"),
    attack = self:createControl("GainBias", "attack"),
    decay  = self:createControl("GainBias", "decay")
  }

  return self._controls
end

function Strike:onLoadGraph(channelCount)
end

function Strike:onLoadViews(objects, branches)
  local controls, views = {}, {
    expanded  = { "strike", "lift", "decay" },
    collapsed = { "strike" },

    lift      = { "lift", "Q" },
    decay     = { "attack", "decay" }
  }

  controls.strike = Gate {
    button      = "strike",
    description = "Hit Me!",
    branch      = branches.strike,
    comparator  = objects.strike
  }

  controls.lift = GainBias {
    button        = "lift",
    description   = "How Bright?",
    branch        = branches.lift,
    gainbias      = objects.lift,
    range         = objects.liftRange,
    biasMap       = Encoder.getMap("[0,1]"),
    biasUnits     = app.unitNone,
    biasPrecision = 1,
    initialBias   = config.initialLift
  }

  controls.Q = GainBias {
    button        = "Q",
    description   = "Resonance",
    branch        = branches.Q,
    gainbias      = objects.Q,
    range         = objects.QRange,
    biasMap       = Encoder.getMap("[0,1]"),
    biasUnits     = app.unitNone,
    biasPrecision = 1,
    initialBias   = config.initialQ
  }

  controls.attack = GainBias {
    button        = "attack",
    description   = "Attack Time",
    branch        = branches.attack,
    gainbias      = objects.attack,
    range         = objects.attackRange,
    biasMap       = Encoder.getMap("[0,10]"),
    biasUnits     = app.unitSecs,
    biasPrecision = 2,
    initialBias   = config.initialAttack
  }

  controls.decay = GainBias {
    button        = "decay",
    description   = "Decay Time",
    branch        = branches.decay,
    gainbias      = objects.decay,
    range         = objects.decayRange,
    biasMap       = Encoder.getMap("[0,10]"),
    biasUnits     = app.unitSecs,
    biasPrecision = 2,
    initialBias   = config.initialDecay
  }

  return controls, views
end