local Class = require "Base.Class"
local MultiVoice = require "Polygon.MultiVoice"

return function (voiceCount, title, mnemonic, oscType, extraOscCount)
  local NVoiceUnit = Class {}
  NVoiceUnit:include(MultiVoice)

  function NVoiceUnit:init(args)
    args.title         = title
    args.mnemonic      = mnemonic
    args.voiceCount    = voiceCount
    args.oscType       = oscType
    args.extraOscCount = extraOscCount

    MultiVoice.init(self, args)
  end

  return NVoiceUnit
end
