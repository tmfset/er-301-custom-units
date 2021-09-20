local app = app
local polygon = require "polygon.libpolygon"
local Class = require "Base.Class"
local Base = require "Unit.ViewControl.EncoderControl"

local PagedViewControl = Class {}
PagedViewControl:include(Base)

function PagedViewControl:init(args)
  Base.init(self, args.name)

  self.subViews       = {}
  self.pages          = {}
  self.pageIndexByKey = {}

  self.subGraphic = polygon.PagedSubView();
end

function PagedViewControl:onRemove()
  for _, subView in ipairs(self.subViews) do
    subView:onRemove()
  end
end

function PagedViewControl:addSubView(subView)
  local nextIndex = #self.subViews + 1

  self.subViews[nextIndex]          = subView
  self.pages[nextIndex]             = subView.name
  self.pageIndexByKey[subView.name] = nextIndex

  subView:setViewControl(self)
  self.subGraphic:addChild(subView.graphic)
end

function PagedViewControl:currentSubView()
  local currentPage = self.subGraphic:currentPage()
  return self.subViews[currentPage + 1]
end

function PagedViewControl:onCursorEnter()
  self:currentSubView():onCursorEnter()
  Base.onCursorEnter(self)
end

function PagedViewControl:zeroPressed()
  return self:currentSubView():zeroPressed()
end

function PagedViewControl:cancelReleased()
  return self:currentSubView():cancelReleased()
end

function PagedViewControl:subReleased(i, shifted)
  if shifted then return false end
  return self:currentSubView():subReleased(i, shifted)
end

function PagedViewControl:subPressed(i, shifted)
  if shifted then return false end
  return self:currentSubView():subPressed(i)
end

function PagedViewControl:encoder(change, shifted)
  return self:currentSubView():encoder(change, shifted)
end

function PagedViewControl:getFloatingMenuItems()
  return self.pages
end

function PagedViewControl:getPageName(i)
  return self.pages[i] or ""
end

function PagedViewControl:getPageIndex(choice)
  return self.pageIndexByKey[choice] or 1
end

function PagedViewControl:getFloatingMenuDefaultChoice()
  return self:getPageName(self.subGraphic:currentPage() + 1)
end

function PagedViewControl:onFloatingMenuEnter()
  self:unfocusSubView()
end

function PagedViewControl:onFloatingMenuChange(choice)
  self:unfocusSubView()
  self.subGraphic:setPage(self:getPageIndex(choice) - 1)
end

function PagedViewControl:onFloatingMenuSelection(choice)
  self.subGraphic:setPage(self:getPageIndex(choice) - 1)
end

function PagedViewControl:unfocusSubView()
  self:currentSubView():setFocusedPosition(nil)
  self:unfocus()
end

return PagedViewControl