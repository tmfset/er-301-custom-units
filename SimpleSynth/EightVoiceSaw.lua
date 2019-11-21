local Class = require "Base.Class"
local MultiVoice = require "SimpleSynth.MultiVoice"

local EightVoiceSaw = Class {}
EightVoiceSaw:include(MultiVoice)

function EightVoiceSaw:init(args)
  args.title = "Eight Voice Saw"
  args.mnemonic = "8VS"
  args.voiceCount = 8
  MultiVoice.init(self, args)
end

return EightVoiceSaw