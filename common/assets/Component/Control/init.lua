local Class = require "Base.Class"

local Control = Class {}

function Control:init(args)
end

function Control:onFocus() end
function Control:onUnfocus() end

function Control:onPress() end
function Control:onRelease() end

function Control:onZeroPress() end
function Control:onZeroRelease() end

function Control:onCancelPress() end
function Control:onCancelRelease() end

function Control:onDialPress() end
function Control:encoder(change, shifted) end

return Control
