local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local Gate = require "Unit.ViewControl.Gate"

local And = Class {}
And:include(Unit)

function And:init(args)
  args.title = "And"
  args.mnemonic = "&"
  Unit.init(self, args)
end

function And:onLoadGraph(channelCount)
  if channelCount == 2 then
    self:loadStereoGraph()
  else
    self:loadMonoGraph()
  end
end

function And:addComparator(name, mode, default)
  local gate = self:addObject(name, app.Comparator())
  gate:setMode(mode)
  self:addMonoBranch(name, gate, "In", gate, "Out")
  if default then
    gate:setOptionValue("State", default)
  end
  return gate
end

function And:loadMonoGraph()
  local left  = self:addComparator("left", app.COMPARATOR_GATE, 0)
  local right = self:addComparator("right", app.COMPARATOR_GATE, 0)

  local op = self:addObject("op", lojik.And())
  connect(left,  "Out", op,   "Left")
  connect(right, "Out", op,   "Right")
  connect(op,    "Out", self, "Out1")
end

function And:loadStereoGraph()
  self:loadMonoGraph()
  connect(self.objects.op, "Out", self, "Out2")
end

local views = {
  expanded = { "left", "right" },
  collapsed = {}
}

function And:onLoadViews(objects, branches)
  local controls = {}

  controls.left = Gate {
    button = "left",
    description = "Left",
    branch = branches.left,
    comparator = objects.left
  }

  controls.right = Gate {
    button = "right",
    description = "Right",
    branch = branches.right,
    comparator = objects.right
  }

  return controls, views
end

return And
