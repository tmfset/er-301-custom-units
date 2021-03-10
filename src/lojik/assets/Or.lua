local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"
local Gate = require "Unit.ViewControl.Gate"

local Or = Class {}
Or:include(Unit)

function Or:init(args)
  args.title = "Or"
  args.mnemonic = "|"
  Unit.init(self, args)
end

function Or:onLoadGraph(channelCount)
  if channelCount == 2 then
    self:loadStereoGraph()
  else
    self:loadMonoGraph()
  end
end

function Or:loadMonoGraph()
  local left  = self:addComparator("left", app.COMPARATOR_GATE, 0)
  local right = self:addComparator("right", app.COMPARATOR_GATE, 0)

  local op = self:addObject("op", lojik.Or())
  connect(left,  "Out", op,   "Left")
  connect(right, "Out", op,   "Right")
  connect(op,    "Out", self, "Out1")
end

function Or:loadStereoGraph()
  self:loadMonoGraph()
  connect(self.objects.op, "Out", self, "Out2")
end

local views = {
  expanded = { "left", "right" },
  collapsed = {}
}

function Or:onLoadViews(objects, branches)
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

return Or
