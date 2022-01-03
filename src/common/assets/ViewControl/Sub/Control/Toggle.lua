local app   = app
local Class = require "Base.Class"
local Base  = require "common.assets.ViewControl.SubControl"

local Toggle = Class {}
Toggle:include(Base)

function Toggle:init(args)
  Base.init(self, args)
  self.option = args.option
  self.option:enableSerialization()

  self.valueOn = args.on
  self.valueOff = args.off

  self.onPress = function ()
    self:toggle()
    self:updateViewState()
  end

  local column = args.column or app.BUTTON2_CENTER
  local row    = args.row or app.GRID5_CENTER3

  self.indicator = app.BinaryIndicator(0, 0, 32, 32)
  self.indicator:setCenter(column, row)
  self:addGraphic(self.indicator)
end

function Toggle:onCursorEnter()
  self:updateViewState()
  return Base.onCursorEnter(self)
end

function Toggle:isOn()
  return self.option:value() == self.valueOn
end

function Toggle:toggle()
  if self:isOn() then
    self.option:set(self.valueOff)
  else
    self.option:set(self.valueOn)
  end
end

function Toggle:updateViewState()
  if self:isOn() then
    self.indicator:on()
  else
    self.indicator:off()
  end
end

return Toggle
