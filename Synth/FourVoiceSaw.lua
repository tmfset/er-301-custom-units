local Class = require "Base.Class"
local MultiVoice = require "Synth.MultiVoice"

local ThreeVoiceSaw = Class {}
ThreeVoiceSaw:include(MultiVoice)

function ThreeVoiceSaw:init(args)
  args.title = "Four Voice Saw"
  args.mnemonic = "4VS"
  args.voiceCount = 4
  MultiVoice.init(self, args)
end

return ThreeVoiceSaw