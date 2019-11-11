local Class = require "Base.Class"
local MultiVoice = require "Synth.MultiVoice"

local ThreeVoiceSaw = Class {}
ThreeVoiceSaw:include(MultiVoice)

function ThreeVoiceSaw:init(args)
  args.title = "Three Voice Saw"
  args.mnemonic = "3VS"
  args.voiceCount = 3
  args.roundRobin = true
  MultiVoice.init(self, args)
end

return ThreeVoiceSaw