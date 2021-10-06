local app     = app
local polygon = require "polygon.libpolygon"
local Class   = require "Base.Class"
local Base    = require "polygon.SplitViewControl"

local PagedViewControl = Class {}
PagedViewControl:include(Base)

function PagedViewControl:init(args)
  Base.init(self, args)

  self.subViews       = {}
  self.pages          = {}
  self.pageIndexByKey = {}

  self.subGraphic = polygon.PagedSubView();
end

function PagedViewControl:attachFollower(follower)
  self.follower = follower
end

function PagedViewControl:updateFollower(pageIndex)
  if self.follower then self.follower:updatePageIndex(pageIndex, false) end
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

function PagedViewControl:subView()
  local currentPage = self.subGraphic:currentPage()
  return self.subViews[currentPage + 1]
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
  self:updatePageIndex(self:getPageIndex(choice), true)
end

function PagedViewControl:onFloatingMenuSelection(choice)
  self:updatePageIndex(self:getPageIndex(choice), true)
  self:focus()
end

function PagedViewControl:updatePageIndex(pageIndex, propogate)
  self.subGraphic:setPage(pageIndex - 1)
  if propogate then self:updateFollower(pageIndex) end
end

return PagedViewControl