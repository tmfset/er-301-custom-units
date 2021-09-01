local app = app
local polygon = require "polygon.libpolygon"
local Class = require "Base.Class"
local ViewControl = require "Unit.ViewControl"

local PagedViewControl = Class {}
PagedViewControl:include(ViewControl)

function PagedViewControl:init(args)
  ViewControl.init(self)

  self.subViews = args.subViews or {}

  self.menuItems = {}

  self.subGraphic = polygon.PagedSubView();
  for _, subView in ipairs(self.subViews) do
    -- TODO: Do better.
    self.menuItems[#self.menuItems + 1] = subView.description:getText()

    subView:setViewControl(self)
    self.subGraphic:addChild(subView.graphic)
  end
end

function PagedViewControl:onRemove()
  for _, subViews in ipairs(self.subViews) do
    viewControl:onRemove()
  end
end

function PagedViewControl:subReleased(i, shifted)
  if shifted and i == 3 then
    self.subGraphic:switchPage()
    return true
  end

  local currentPage = self.subGraphic:currentPage()
  local subView = self.subViews[currentPage + 1]
  return subView:subReleased(i, shifted)
end

function PagedViewControl:subPressed(i, shifted)
  local currentPage = self.subGraphic:currentPage()
  local subView = self.subViews[currentPage + 1]
  return subView:subPressed(i)
end

function PagedViewControl:getFloatingMenuItems()
  return self.menuItems
end

function PagedViewControl:onFloatingMenuEnter()
  self.subGraphic:setPage(0)
end

function PagedViewControl:onFloatingMenuChange(change, index)
  -- The first index is always cancel
  if index > 0 then
    self.subGraphic:setPage(index - 1)
  end
end

function PagedViewControl:spotPressed(spotIndex, shifted, isFocusedPress)
  if isFocusedPress then
    self.subGraphic:switchPage()
    return true
  end

  --if shifted then return true end
  return ViewControl.spotPressed(self, spotIndex, shifted, isFocusedPress)
end

-- function PagedViewControl:spotReleased(spotIndex, shifted)
--   if shifted then
--     self.subGraphic:switchPage()
--     return true
--   end
--   return ViewControl.spotReleased(self, spotIndex, shifted)
-- end

return PagedViewControl