local Class = require "Base.Class"

local Spot = Class {}

function Spot:init(args)
  self:setClassName("Spot")
end

function Spot:getMainGraphic() end
function Spot:getSubGraphic() end

function Spot:getFloatingMenuItems() end
function Spot:onFloatingMenuSelect(choice) end

function Spot:onFocused() end
function Spot:onUnfocused() end

function Spot:onCursorEnter() end
function Spot:onCursorLeave() end

function Spot:zeroPressed() end
function Spot:zeroReleased() end

function Spot:cancelPressed() end
function Spot:cancelReleased() end

function Spot:mainPressed() end
function Spot:mainReleased() end

function Spot:subPressed(i, shifted) end
function Spot:subReleased(i, shifted) end

function Spot:dialPressed(shifted) end

function Spot:encoder(change, shifted) end

return Spot
