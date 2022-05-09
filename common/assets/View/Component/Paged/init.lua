local app    = app
local Class  = require "Base.Class"
local Base   = require "common.assets.View.Component"
local common = require "common.lib"

local Paged = Class {}
Paged:include(Base)

--- Initialize a paged component.
--
-- A paged component is a list of view components that
-- display one at a time. The paged component wraps each
-- child and delegates callbacks to the active page.
function Paged:init(args)
  Base.init(self, args)
  self:setClassName("Component.Paged")

  self._pages       = {}
  self._indexByName = {}

  self._graphic = common.PagedGraphic()
end

--- Add a new page.
-- @param name      The page name.
-- @param component The component.
function Paged:addPage(name, component)
  local index = #self._pages + 1

  self._pages[index]      = { name = name, component = component }
  self._indexByName[name] = index

  self._graphic:addChild(component:graphic())
end

-- Set the current page by name.
-- @param name The page name.
function Paged:setCurrentPageByName(name)
  self:setCurrentPageByIndex(self._indexByPage[name])
end

--- Set the current page by index.
-- @param index The page index.
function Paged:setCurrentPageByIndex(index)
  self._graphic:setPage(index - 1)
  self:emitSignal("onPageChange", index)
end

--- Follow page changes on another instance.
-- @param other The other instance follow.
function Paged:follow(other)
  other:subscribe("onPageChange", self.setCurrentPageByIndex)
end

--- The component at a given index.
-- @param index The index.
function Paged:componentAtIndex(index)
  return self._pages[index].component
end

--- The current page index.
function Paged:currentPageIndex()
  return self._graphic:currentPage()
end

-- The currently visibly component.
function Paged:currentComponent()
  return self:componentAtIndex(self:currentPageIndex())
end

------------
-- Inherited.
------------

function Paged:onFocus() self:currentComponent():onFocus() end
function Paged:onUnfocus() self:currentComponent():onUnfocus() end

function Paged:onPress() self:currentComponent():onPress() end
function Paged:onRelease() self:currentComponent():onRelease() end
function Paged:onHold() self:currentComponent():onHold() end

function Paged:onEncoderMove() self:currentComponent():onEncoderMove() end
function Paged:onEncoderPress() self:currentComponent():onEncoderPress() end

function Paged:onCancelPress() self:currentComponent():onCancelPress() end
function Paged:onCancelRelease() self:currentComponent():onCancelRelease() end

function Paged:onZeroPress() self:currentComponent():onZeroPress() end
function Paged:onZeroRelease() self:currentComponent():onZeroRelease() end

return Paged
