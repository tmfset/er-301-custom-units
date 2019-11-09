-- luacheck: globals app os verboseLevel connect tie
local Class = require "Base.Class"
local PolySynth = require "Poly.PolySynth"

local PolySynth3 = Class{}
PolySynth3:include(PolySynth)

function PolySynth3:init(args)
  args.voiceCount = 3
  PolySynth.init(self, args)
end

return PolySynth3