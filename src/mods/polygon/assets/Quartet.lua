local polygon = require "polygon.libpolygon"
local Class   = require "Base.Class"
local Polygon = require "polygon.Polygon"

local Quartet = Class {}
Quartet:include(Polygon)

function Quartet:init(args)
  args.title    = "4-tet"
  args.mnemonic = "4tet"
  args.ctor     = polygon.Quartet
  Polygon.init(self, args)
end

return Quartet