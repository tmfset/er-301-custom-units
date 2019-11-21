local Class = require "Base.Class"
local MultiVoice = require "SimpleSynth.MultiVoice"

local FourVoiceSaw = Class {}
FourVoiceSaw:include(MultiVoice)

function FourVoiceSaw:init(args)
  args.title = "Four Voice Saw"
  args.mnemonic = "4VS"
  args.voiceCount = 4
  MultiVoice.init(self, args)
end

return FourVoiceSaw