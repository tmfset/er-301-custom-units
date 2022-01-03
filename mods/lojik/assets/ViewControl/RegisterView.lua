local app   = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"

local UnitShared = require "common.assets.UnitShared"
local Base       = require "common.assets.ViewControl.Split.Paged"
local GainBias   = require "common.assets.ViewControl.SubView.GainBias"

local ply = app.SECTION_PLY

local RegisterView = Class {
  type    = "RegisterView",
  canEdit = false,
  canMove = true
}
RegisterView:include(Base)
RegisterView:include(UnitShared)

function RegisterView:init(args)
  Base.init(self, args)
  self.register = args.register or app.logError("%s.init: missing register instance.", self)

  self:addSubView(GainBias {
    name           = "Offset",
    branch         = args.offset.branch,
    gainBias       = args.offset.gainBias,
    gainEncoderMap = self.intMap(-lojik.REGISTER_MAX, lojik.REGISTER_MAX),
    biasEncoderMap = self.intMap(0, lojik.REGISTER_MAX),
  })

  self:addSubView(GainBias {
    name           = "Shift",
    branch         = args.shift.branch,
    gainBias       = args.shift.gainBias,
    gainEncoderMap = self.intMap(-lojik.REGISTER_MAX, lojik.REGISTER_MAX),
    biasEncoderMap = self.intMap(0, lojik.REGISTER_MAX),
  })

  local width = args.width or 2
  self.graphic = lojik.RegisterMainView(self.register, 0, 0, width * ply, app.SCREEN_HEIGHT)
  self:setControlGraphic(self.graphic)
  for i = 1, width do
    self:addSpotDescriptor {
      center = (i - 0.5) * ply
    }
  end
end

function RegisterView:updatePageIndex(pageIndex, propogate)
  self.graphic:setCursorSelection(pageIndex)
  Base.updatePageIndex(self, pageIndex, propogate)
end

return RegisterView