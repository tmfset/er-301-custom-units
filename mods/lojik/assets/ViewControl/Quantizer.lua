local app   = app
local lojik = require "lojik.liblojik"
local Class = require "Base.Class"

local Base     = require "common.assets.ViewControl.Split.Paged"
local Scale    = require "common.assets.ViewControl.SubView.Scale"

local Quantizer = Class {
  type    = "Quantizer",
  canEdit = false,
  canMove = true
}
Quantizer:include(Base)

function Quantizer:init(args)
  Base.init(self, args)
  self.quantizer = args.quantizer or app.logError("%s.init: missing quantizer.", self)

  self:addSubView(Scale {
    name        = "Scale",
    branch      = args.branch,
    gainBias    = args.gainBias,
    gainDialMap = args.gainDialMap,
    scaleSource = self.quantizer
  })

  self.graphic = lojik.QuantizerView(self.quantizer)
  self:setControlGraphic(self.graphic)

  for i = 1, self.graphic:plyWidth() do
    self:addSpotDescriptor {
      center = (i - 0.5) * app.SECTION_PLY
    }
  end
end

return Quantizer