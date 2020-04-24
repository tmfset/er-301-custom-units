-- luacheck: globals app connect
local app = app
local Class = require "Base.Class"
local Unit = require "Unit"
local Pitch = require "Unit.ViewControl.Pitch"
local Fader = require "Unit.ViewControl.Fader"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"
local OutputScope = require "Unit.ViewControl.OutputScope"
local Encoder = require "Encoder"
local SamplePool = require "Sample.Pool"
local SamplePoolInterface = require "Sample.Pool.Interface"
local SlicingView = require "SlicingView"
local Task = require "Unit.MenuControl.Task"
local MenuHeader = require "Unit.MenuControl.Header"
local ply = app.SECTION_PLY

local QPer = Class {}
QPer:include(Unit)

function QPer:init(args)
  args.title      = "QPer"
  args.mnemonic   = "qpr"
  Unit.init(self, args)
end

function QPer:onLoadGraph(channelCount)
end

function QPer:onLoadViews(objects, branches)
end

return QPer
