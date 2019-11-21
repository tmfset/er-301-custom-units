local Class = require "Base.Class"
local MultiVoice = require "SimpleSynth.MultiVoice"

local ThreeVoiceSaw = Class {}
ThreeVoiceSaw:include(MultiVoice)

function ThreeVoiceSaw:init(args)
  args.title = "Three Voice Saw"
  args.mnemonic = "3VS"
  args.voiceCount = 3
  MultiVoice.init(self, args)
end

return ThreeVoiceSaw