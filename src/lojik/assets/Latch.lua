local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local Gate = require "Unit.ViewControl.Gate"

local Latch = Class {}
Latch:include(Unit)

function Latch:init(args)
  args.title = "Latch"
  args.mnemonic = "lx"
  Unit.init(self, args)
end

function Latch:onLoadGraph(channelCount)
  if channelCount == 2 then
    self:loadStereoGraph()
  else
    self:loadMonoGraph()
  end
end

function Latch:loadMonoGraph()
  local set = self:addObject("set", app.Comparator())
  set:setGateMode()

  local reset = self:addObject("reset", app.Comparator())
  reset:setGateMode()

  local latch = self:addObject("latch", lojik.Latch())

  connect(set, "Out", latch, "Set")
  connect(reset, "Out", latch, "Reset")

  self:addMonoBranch("set", set, "In", set, "Out")
  self:addMonoBranch("reset", reset, "In", reset, "Out")

  connect(latch, "Out", self, "Out1")
end

function Latch:loadStereoGraph()
  self:loadMonoGraph()
  connect(self.objects.latch, "Out", self, "Out2")
end

local views = {
  expanded = {
    "set",
    "reset"
  },
  collapsed = {}
}

function Latch:onLoadViews(objects, branches)
  local controls = {}

  controls.set = Gate {
    button = "set",
    description = "Set",
    branch = branches.set,
    comparator = objects.set
  }

  controls.reset = Gate {
    button = "reset",
    description = "Reset",
    branch = branches.reset,
    comparator = objects.reset
  }

  return controls, views
end

return Latch
