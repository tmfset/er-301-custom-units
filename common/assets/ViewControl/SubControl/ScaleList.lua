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

  local param  = args.parameter or app.logError("%s: missing parameter.", self)
  local source = args.source or app.logError("%s: missing source.", self)

  local column    = args.column    or app.BUTTON1_CENTER
  local row       = args.row       or app.GRID5_CENTER1

  self.list = common.ScaleListView(source, param)
  self.list:setAttributes(
    common.unitNone,
    common.LinearDialMap(0, source:getScaleBookSize(), 0, 0.001, false, 1, 0.25, 0.1, 0.1)
  );
  self.list:setCenter(column, row)
  self:addGraphic(self.list)

  self.encoderState = args.encoderState or Encoder.Coarse
end

function ScaleList:onFocus()
  if not self:hasParentFocus("encoder") then self:focusParent() end
  Encoder.set(self.encoderState)
end

function ScaleList:onDialPress()
  if self.encoderState == Encoder.Coarse then
    self.encoderState = Encoder.Fine
  else
    self.encoderState = Encoder.Coarse
  end
  Encoder.set(self.encoderState)
end

function ScaleList:getCursorController()
  return self.list
end

function ScaleList:onEncoder(change, shifted)
  self.list:encoder(change, shifted, self.encoderState == Encoder.Fine)
end

return ScaleList