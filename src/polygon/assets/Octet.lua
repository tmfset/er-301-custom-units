local polygon = require "polygon.libpolygon"
local Class   = require "Base.Class"
local Polygon = require "polygon.Polygon"

local Octet = Class {}
Octet:include(Polygon)

function Octet:init(args)
  args.title    = "8-tet"
  args.mnemonic = "8tet"
  args.ctor     = polygon.Octet
  Polygon.init(self, args)
end

return Octet