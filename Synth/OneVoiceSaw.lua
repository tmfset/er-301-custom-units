local Class = require "Base.Class"
local MultiVoice = require "Synth.MultiVoice"

local OneVoiceSaw = Class {}
OneVoiceSaw:include(MultiVoice)

function OneVoiceSaw:init(args)
  args.title = "One Voice Saw"
  args.mnemonic = "1VS"
  args.voiceCount = 1
  MultiVoice.init(self, args)
end

return OneVoiceSaw