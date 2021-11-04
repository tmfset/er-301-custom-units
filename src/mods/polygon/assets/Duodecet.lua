local polygon = require "polygon.libpolygon"
local Class   = require "Base.Class"
local Polygon = require "polygon.Polygon"

local Duodecet = Class {}
Duodecet:include(Polygon)

function Duodecet:init(args)
  args.title    = "12-tet"
  args.mnemonic = "12tet"
  args.ctor     = polygon.Duodecet
  Polygon.init(self, args)
end

return Duodecet