local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"

local Trig = Class {}
Trig:include(Unit)

function Trig:init(args)
  args.title = "Trig"
  args.mnemonic = "t"
  Unit.init(self, args)
end

function Trig:onLoadGraph(channelCount)
  if channelCount == 2 then
    self:loadStereoGraph()
  else
    self:loadMonoGraph()
  end
end

function Trig:loadMonoGraph()
  local op = self:addObject("op", lojik.Trig())
  connect(self, "In1", op, "In")
  connect(op, "Out", self, "Out1")
end

function Trig:loadStereoGraph()
  self:loadMonoGraph()
  connect(self.objects.op, "Out", self, "Out2")
end

local views = {
  expanded = {},
  collapsed = {}
}

function Trig:onLoadViews(objects, branches)
  local controls = {}

  return controls, views
end

return Trig
