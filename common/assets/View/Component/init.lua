local Class = require "Base.Class"
local Observable = require "Base.Observable"

local Component = Class {}
Component:include(Observable)

--- Initialize a view component.
--
-- A view component is a smaller piece of an overall view
-- control. It encapsulates reusable view (graphic) and
-- control logic.
function Component:init(args)
  Observable.init(self)
  self:setClassName("Component")
end

--- When the component is focused.
function Component:onFocus() end
--- When the component loses focus.
function Component:onUnfocus() end

--- When the component is pressed.
function Component:onPress() end
--- When the component is released following a press.
function Component:onRelease() end
--- When the component is held following a press.
function Component:onHold() end

--- When the encoder moves while the component is in focus.
function Component:onEncoderMove() end
--- When the encoder is pressed while the component is in focus.
function Component:onEncoderPress() end

--- When cancel is pressed while the component is in focus.
function Component:onCancelPress() end
--- When cancel is released while the component is in focus.
function Component:onCancelRelease() end

--- When zero is pressed while the component is in focus.
function Component:onZeroPress() end
--- When zero is released while the component is in focus.
function Component:onZeroRelease() end

return Component
