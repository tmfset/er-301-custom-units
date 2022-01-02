local app    = app
local common = require "common.lib"
local Class  = require "Base.Class"
local Base   = require "common.assets.ViewControl.Split"

local Paged = Class {}
Paged:include(Base)

function Paged:init(args)
  Base.init(self, args)

  self.subViews       = {}
  self.pages          = {}
  self.pageIndexByKey = {}

  self.subGraphic = common.PagedGraphic(0, 0, 128, 64);
end

function Paged:attachFollower(follower)
  self.follower = follower
end

function Paged:updateFollower(pageIndex)
  if self.follower then self.follower:updatePageIndex(pageIndex, false) end
end

function Paged:onRemove()
  for _, subView in ipairs(self.subViews) do
    subView:onRemove()
  end
end

function Paged:addSubView(subView)
  local nextIndex = #self.subViews + 1

  self.subViews[nextIndex]          = subView
  self.pages[nextIndex]             = subView.name
  self.pageIndexByKey[subView.name] = nextIndex

  subView:setViewControl(self)
  self.subGraphic:addChild(subView.graphic)
end

function Paged:subView()
  local currentPage = self.subGraphic:currentPage()
  return self.subViews[currentPage + 1]
end

function Paged:getFloatingMenuItems()
  return self.pages
end

function Paged:getPageName(i)
  return self.pages[i] or ""
end

function Paged:getPageIndex(choice)
  return self.pageIndexByKey[choice] or 1
end

function Paged:getFloatingMenuDefaultChoice()
  return self:getPageName(self.subGraphic:currentPage() + 1)
end

function Paged:onFloatingMenuEnter()
  self:unfocusSubView()
end

function Paged:onFloatingMenuChange(choice)
  self:unfocusSubView()
  self:updatePageIndex(self:getPageIndex(choice), true)
end

function Paged:onFloatingMenuSelection(choice)
  self:updatePageIndex(self:getPageIndex(choice), true)
  self:focus()
end

function Paged:updatePageIndex(pageIndex, propogate)
  self.subGraphic:setPage(pageIndex - 1)
  if propogate then self:updateFollower(pageIndex) end
end

return Paged
