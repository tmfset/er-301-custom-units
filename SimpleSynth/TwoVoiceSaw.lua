local Class = require "Base.Class"
local MultiVoice = require "SimpleSynth.MultiVoice"

local TwoVoiceSaw = Class {}
TwoVoiceSaw:include(MultiVoice)

function TwoVoiceSaw:init(args)
  args.title = "Two Voice Saw"
  args.mnemonic = "2VS"
  args.voiceCount = 2
  MultiVoice.init(self, args)
end

return TwoVoiceSaw