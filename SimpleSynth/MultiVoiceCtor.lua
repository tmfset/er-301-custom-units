local Class = require "Base.Class"
local MultiVoice = require "SimpleSynth.MultiVoice"

return function (voiceCount, title, mnemonic, oscType)
  local NVoiceSaw = Class {}
  NVoiceSaw:include(MultiVoice)

  function NVoiceSaw:init(args)
    args.title      = title
    args.mnemonic   = mnemonic
    args.voiceCount = voiceCount
    args.oscType    = oscType

    MultiVoice.init(self, args)
  end

  return NVoiceSaw
end
