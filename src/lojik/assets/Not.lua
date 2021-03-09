local app = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"
local Unit = require "Unit"

local Not = Class {}
Not:include(Unit)

function Not:init(args)
  args.title = "Not"
  args.mnemonic = "!"
  Unit.init(self, args)
end

function Not:onLoadGraph(channelCount)
  if channelCount == 2 then
    self:loadStereoGraph()
  else
    self:loadMonoGraph()
  end
end

function Not:loadMonoGraph()
  local op = self:addObject("op", lojik.Not())
  connect(self, "In1", op, "In")
  connect(op, "Out", self, "Out1")
end

function Not:loadStereoGraph()
  self:loadMonoGraph()
  connect(self.objects.op, "Out", self, "Out2")
end

local views = {
  expanded = {},
  collapsed = {}
}

function Not:onLoadViews(objects, branches)
  local controls = {}

  return controls, views
end

return Not
