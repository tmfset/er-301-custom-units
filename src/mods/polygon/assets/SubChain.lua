local app = app
local Class = require "Base.Class"
local Base = require "polygon.SubControl"

local SubChain = Class {}
SubChain:include(Base)

function SubChain:init(args)
  args.name = args.name or "empty"
  Base.init(self, args)

  self.branch = args.branch or app.logError("%s.addScope: missing branch.", self)

  local column = args.column or app.BUTTON1_CENTER
  local row    = args.row or app.GRID5_CENTER1

  self.scope = app.MiniScope(column, row, 40, 45)
  self.scope:setBorder(1)
  self.scope:setCornerRadius(3, 3, 3, 3)
  self:addGraphic(self.scope)

  self.branch:subscribe("contentChanged", self)
end

function SubChain:contentChanged(chain)
  if not chain == self.branch then return end
  local outlet = chain:getMonitoringOutput(1)
  self.scope:watchOutlet(outlet)
  self.button:setText(chain:mnemonic())
end

function SubChain:onRemove()
  self.branch:unsubscribe("contentChanged", self)
end

function SubChain:onRelease()
  self:unfocusParent()
  self.branch:show()
end

return SubChain