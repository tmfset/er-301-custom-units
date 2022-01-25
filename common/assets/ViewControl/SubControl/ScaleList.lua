local app     = app
local ply     = app.SECTION_PLY

local common  = require "common.lib"
local DialMap = require "common.assets.DialMap"
local Class   = require "Base.Class"
local Encoder = require "Encoder"
local Base    = require "common.assets.ViewControl.SubControl"

local ScaleList = Class {}
ScaleList:include(Base)

function ScaleList:init(args)
  args.label = false
  Base.init(self, args)

  local source = args.source or app.logError("%s: missing source.", self)

  local column    = args.column    or app.BUTTON1_CENTER
  local row       = args.row       or app.GRID5_CENTER1

  self.list = common.ScaleListView(source)
  self.list:setCenter(column, row)
  self:addGraphic(self.list)

  self.encoderState = args.encoderState or Encoder.Coarse
end

function ScaleList:onFocus()
  if not self:hasParentFocus("encoder") then self:focusParent() end
  Encoder.set(self.encoderState)
end

function ScaleList:getCursorController()
  return self.list
end

return ScaleList