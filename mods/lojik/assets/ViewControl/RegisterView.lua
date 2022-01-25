local app   = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"

local UnitShared = require "common.assets.UnitShared"
local Base       = require "common.assets.ViewControl.Split.Paged"
local GainBias   = require "common.assets.ViewControl.SubView.GainBias"
local Scale      = require "common.assets.ViewControl.SubView.Scale"

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
    name        = "Offset",
    branch      = args.offset.branch,
    precision   = 0,
    gainBias    = args.offset.gainBias,
    gainDialMap = args.offset.gainDialMap,
    biasDialMap = args.offset.biasDialMap,
  })

  self:addSubView(GainBias {
    name        = "Shift",
    branch      = args.shift.branch,
    precision   = 0,
    gainBias    = args.shift.gainBias,
    gainDialMap = args.shift.gainDialMap,
    biasDialMap = args.shift.biasDialMap,
  })

  self:addSubView(GainBias {
    name        = "Length",
    branch      = args.length.branch,
    precision   = 0,
    gainBias    = args.length.gainBias,
    gainDialMap = args.length.gainDialMap,
    biasDialMap = args.length.biasDialMap,
  })

  self:addSubView(GainBias {
    name        = "Stride",
    branch      = args.stride.branch,
    precision   = 0,
    gainBias    = args.stride.gainBias,
    gainDialMap = args.stride.gainDialMap,
    biasDialMap = args.stride.biasDialMap,
  })

  self:addSubView(Scale {
    name        = "Quantize",
    branch      = args.quantize.branch,
    precision   = 0,
    gainBias    = args.quantize.gainBias,
    gainDialMap = args.quantize.gainDialMap,
    scaleSource = self.register
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
  self:setMainCursorController(self.graphic:getCursorController(pageIndex - 1));
  Base.updatePageIndex(self, pageIndex, propogate)
end

return RegisterView