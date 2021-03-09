local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local Gate = require "Unit.ViewControl.Gate"

local DLatch = Class {}
DLatch:include(Unit)

function DLatch:init(args)
  args.title = "DLatch"
  args.mnemonic = "dlx"
  Unit.init(self, args)
end

function DLatch:onLoadGraph(channelCount)
  if channelCount == 2 then
    self:loadStereoGraph()
  else
    self:loadMonoGraph()
  end
end

function DLatch:loadMonoGraph()
  local clock = self:addObject("clock", app.Comparator())
  clock:setGateMode()

  local reset = self:addObject("reset", app.Comparator())
  reset:setGateMode()

  local latch = self:addObject("latch", lojik.DLatch())

  connect(self, "In1", latch, "In")
  connect(clock, "Out", latch, "Clock")
  connect(reset, "Out", latch, "Reset")

  self:addMonoBranch("clock", latch, "Clock", clock, "Out")
  self:addMonoBranch("reset", reset, "In", reset, "Out")

  connect(latch, "Out", self, "Out1")
end

function DLatch:loadStereoGraph()
  self:loadMonoGraph()
  connect(self.objects.latch, "Out", self, "Out2")
end

local views = {
  expanded = { "clock", "reset" },
  collapsed = {}
}

function DLatch:onLoadViews(objects, branches)
  local controls = {}

  controls.clock = Gate {
    button = "clock",
    description = "Clock",
    branch = branches.clock,
    comparator = objects.clock
  }

  controls.reset = Gate {
    button = "reset",
    description = "Reset",
    branch = branches.reset,
    comparator = objects.reset
  }

  return controls, views
end

return DLatch
