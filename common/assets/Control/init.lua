local Class      = require "Base.Class"

local Control = Class {}

function Control:init(args)
end

function Control:onFocus() end
function Control:onRemove() end

function Control:onZero() end
function Control:onCancel() end

function Control:onPress(focused) end
function Control:onRelease(focused) end

function Control:onDialPress() end
function Control:onDialMove() end

return Control
