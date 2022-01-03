local app = app
local Class = require "Base.Class"
local Control = require "Package.Menu.Control"

local SloopHeader = Class {}
SloopHeader:include(Control)

function SloopHeader:init(args)
  Control.init(self)
  self:setClassName("Sloop.SloopHeader")

  local description = args.description or "Sloop! : Synchronized Looper"
  self:setInstanceName(description)

  local graphic = app.RichTextBox(description, 10)
  graphic:setJustification(app.justifyCenter)
  graphic:setBorder(0)
  graphic:fitHeight(6 * app.SECTION_PLY + 20)
  self:setControlGraphic(graphic)

  self.isHeader = true
end

return SloopHeader
