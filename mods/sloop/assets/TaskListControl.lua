local app = app
local Class = require "Base.Class"
local MenuControl = require "Unit.MenuControl"
local ply = app.SECTION_PLY

local TaskListControl = Class {}
TaskListControl:include(MenuControl)

function TaskListControl:init(args)
  MenuControl.init(self)
  self:setClassName("Sloop.TaskListControl")
  -- required arguments
  local description = args.description or app.logError("%s.init: description is missing.", self)
  self:setInstanceName(description)
  local tasks = args.tasks or app.logError("%s.init: 'tasks' is missing.", self)
  local descWidth = args.descriptionWidth or 1

  self.description = description
  self.tasks = tasks
  self.descWidth = descWidth

  local graphic = app.RichTextBox(description, 10)
  --graphic:setBorder(1)
  --graphic:setBorderColor(app.GRAY7)
  --graphic:setCornerRadius(3, 3, 3, 3)
  graphic:setMargins(3, 1, #tasks * ply, 1)
  graphic:fitHeight((#tasks + descWidth) * ply - 2)

  for i, task in ipairs(tasks) do
    local button = app.FittedTextBox(task.description)
    button:setCenter(app.getButtonCenter(i + descWidth), graphic.mHeight // 2)
    graphic:addChild(button)
    -- label:fitHeight(ply - 2)
    -- graphic:addChild(label)
    -- label:setForegroundColor(app.GRAY5)
    -- label:setJustification(app.justifyCenter)
    -- label:setCenter(app.getButtonCenter(i + descWidth), h)
  end

  --local instructions = app.DrawingInstructions()
  --instructions:color(app.GRAY7)
  --instructions:vline(descWidth * ply - 2, 1, graphic.mHeight - 1)
  --local drawing = app.Drawing(0, 0, 256, 64)
  --drawing:add(instructions)
  --graphic:addChild(drawing)

  self:setControlGraphic(graphic)
end

function TaskListControl:onReleased(i, shifted)
  local task = self.tasks[i - self.descWidth];
  if task then
    self:callUp("hide")
    task.task();
  end

  -- if shifted then return false end
  -- if i > self.descWidth then
  --   local t = i - self.descWidth - self.offset
  --   if t <= #self.choices then
  --     if self.muteOnChange then
  --       local chain = self.parent.unit.chain
  --       local wasMuted = chain:muteIfNeeded()
  --       self.option:set(choice)
  --       self:update()
  --       chain:unmuteIfNeeded(wasMuted)
  --     else
  --       self.option:set(choice)
  --       self:update()
  --     end
  --   end
  -- end
  return true
end

return TaskListControl
