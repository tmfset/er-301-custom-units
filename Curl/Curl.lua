-- luacheck: globals app connect
local app = app
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local OutputScope = require "Unit.ViewControl.OutputScope"
local ply = app.SECTION_PLY

local config = require "Curl.defaults"

local Curl = Class {}
Curl:include(Unit)

function Curl:init(args)
  args.title    = "Curl"
  args.mnemonic = "crl"

  self.depth = 4

  Unit.init(self, args)
end

function Curl:createControl(type, name)
  local control = self:createObject(type, name)
  return control
end

function Curl:memoize(table, key, op)
  self["memory"] = self["memory"] or {}
  local memory = self["memory"]

  memory[table] = memory[table] or {}
  local cache = memory[table]

  if not cache[key] then
    cache[key] = op()
  end
  return cache[key]
end

function Curl:mControl(key, op)
  return self:memoize("controls", key, op)
end

function Curl:range(input)
  local range = self:createObject("MinMax", input:name().."Range")
  connect(input, "Out", range, "In")
  return range
end

function Curl:branch(control)
  return self:createMonoBranch(control:name(), control, "In", control, "Out")
end

local createMap = function (min, max, superCourse, course, fine, superFine, rounding)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCourse, course, fine, superFine)
  map:setRounding(rounding)
  return map
end

function Curl:gainControl()
  return self:mControl("gain", function ()
    return self:createObject("GainBias", "gain")
  end)
end

function Curl:gainView()
  local control = self:gainControl()

  return GainBias {
    button        = "gain",
    description   = "Input Gain",
    branch        = self:branch(control),
    gainbias      = control,
    range         = self:range(control),
    biasMap       = createMap(0, 10, 1.0, 0.1, 0.01, 0.001, 0.001),
    biasUnits     = app.unitNone,
    initialBias   = config.initial.gain
  }
end

function Curl:biasControl()
  return self:mControl("bias", function ()
    return self:createObject("GainBias", "bias")
  end)
end

function Curl:biasView()
  local control = self:biasControl()

  return GainBias {
    button        = "bias",
    description   = "Input Bias",
    branch        = self:branch(control),
    gainbias      = control,
    range         = self:range(control),
    biasMap       = createMap(-10, 10, 0.1, 0.01, 0.001, 0.001, 0.001),
    biasUnits     = app.unitNone,
    initialBias   = config.initial.bias
  }
end

function Curl:limitControl()
  return self:mControl("limit", function ()
    return self:createControl("GainBias", "limit")
  end)
end

function Curl:limitView()
  local control = self:limitControl()

  return GainBias {
    button        = "limit",
    description   = "Amplitude Limit",
    branch        = self:branch(control),
    gainbias      = control,
    range         = self:range(control),
    biasMap       = createMap(0, 1, 0.1, 0.01, 0.001, 0.001, 0.001),
    biasUnits     = app.unitNone,
    biasPrecision = 3,
    initialBias   = config.initial.limit
  }
end

function Curl:reflectControl()
  return self:mControl("reflect", function ()
    return self:createControl("GainBias", "reflect")
  end)
end

function Curl:reflectView()
  local control = self:reflectControl()

  return GainBias {
    button        = "reflect",
    description   = "Reflection Point",
    branch        = self:branch(control),
    gainbias      = control,
    range         = self:range(control),
    biasMap       = createMap(-1, 1, 0.1, 0.01, 0.001, 0.001, 0.001),
    biasUnits     = app.unitNone,
    biasPrecision = 3,
    initialBias   = config.initial.reflect
  }
end

function Curl:wetControl()
  return self:mControl("wet", function ()
    return self:createControl("GainBias", "wet")
  end)
end

function Curl:wetView()
  local control = self:wetControl()

  return GainBias {
    button        = "wet",
    description   = "Wet/Dry",
    branch        = self:branch(control),
    gainbias      = control,
    range         = self:range(control),
    biasMap       = createMap(0, 1, 0.1, 0.01, 0.001, 0.001, 0.001),
    biasUnits     = app.unitNone,
    biasPrecision = 3,
    initialBias   = config.initial.wet
  }
end

function Curl:mSignal(key, op)
  return self:memoize("signals", key, op)
end

function Curl:leftInputSignal()
  return self:mSignal("leftInput", function ()
    local gain = self:createObject("Gain", "leftInput")
    connect(self, "In1", gain, "In")
    return gain
  end)
end

function Curl:rightInputSignal()
  return self:mSignal("rightInput", function ()
    local gain = self:createObject("Gain", "rightInput")
    connect(self, "In2", gain, "In")
    return gain
  end)
end

function Curl:withGainBias(input)
  return self:sum(
    self:mult(input, self:gainControl()),
    self:biasControl()
  )
end

function Curl:mConst(value)
  return self:memoize("constants", value, function ()
    local const = self:createObject("Constant", "Constant"..value)
    const:hardSet("Value", value)
    return const
  end)
end

function Curl:cGainBias(input, gain, bias, suffix)
  local gb = self:createObject("GainBias", "gainBias"..suffix)
  gb:hardSet("Gain", gain)
  gb:hardSet("Bias", bias)
  connect(input, "Out", gb, "In")
  return gb
end

function Curl:clip(input)
  local name = "Clip"..input:name()
  return self:memoize("clip", name, function ()
    local clip = self:createObject("Clipper", name, 0, 1)
    connect(input, "Out", clip, "In")
    return clip
  end)
end

function Curl:cGate(input)
  local name = "Gate"..input:name()
  return self:memoize("gate", name, function ()
    local gate = self:createObject("Comparator", name)
    gate:setGateMode()
    connect(input, "Out", gate, "In")
    return gate
  end)
end

function Curl:pRectify(input)
  local name = "PRectify"..input:name()
  return self:memoize("pRectify", name, function ()
    local rectify = self:createObject("Rectify", name)
    rectify:setOptionValue("Type", 1) -- positiveHalf
    connect(input, "Out", rectify, "In")
    return rectify
  end)
end

function Curl:nRectify(input)
  local name = "NRectify"..input:name()
  return self:memoize("nRectify", name, function ()
    local rectify = self:createObject("Rectify", name)
    rectify:setOptionValue("Type", 2) -- negativeHalf
    connect(input, "Out", rectify, "In")
    return rectify
  end)
end

function Curl:logicalOr(left, right)
  return self:clip(self:sum(left, right))
end

function Curl:logicalNot(input)
  return self:memoize("not", input:name(), function ()
    return self:cGainBias(input, -1, 1, "Not"..input:name())
  end)
end

function Curl:negate(input)
  return self:mult(input, self:mConst(-1))
end

function Curl:cFold(input, threshold, lGain, uGain)
  local fold = self:createObject("Fold")
  connect(input,     "Out", fold, "In")
  connect(threshold, "Out", fold, "Threshold")

  connect(self:mConst(lGain), "Out", fold, "Lower Gain")
  connect(self:mConst(uGain), "Out", fold, "Upper Gain")

  return fold
end

function Curl:pick(gate, left, right)
  local notGate   = self:logicalNot(gate)
  local pickLeft  = self:mult(left, gate)
  local pickRight = self:mult(right, notGate)
  return self:sum(pickLeft, pickRight)
end

function Curl:adapt(input, gain, bias)
  local key = input:name().."Adapter"..gain.."X"..bias
  return self:memoize("adapters", key, function ()
    local adapter = self:createObject("ParameterAdapter", key)
    adapter:hardSet("Gain", gain)
    adapter:hardSet("Bias", bias)
    connect(input, "Out", adapter, "In")
    return adapter
  end)
end

local function append(a, b)
  local out = {}

  if a then
    for i = 0, #a do
      out[#out + 1] = a[i]
    end
  end

  if b then
    for i = 0, #b do
      out[#out + 1] = b[i]
    end
  end

  return out
end

local function slice(values, from, to)
  local out = {}
  for i = math.min(from, to), math.max(from, to), 1 do
    out[#out + 1] = values[i]
  end
  return out
end

local function bRightReduce(values, op)
  if #values == 1 then
    return values[1]
  end

  if #values == 2 then
    return op(values[1], values[2])
  end

  local middle = math.ceil(#values / 2)
  local bottom = slice(values, 1, middle)
  local top = slice(values, middle + 1, #values)
  return op(bRightReduce(bottom, op), bRightReduce(top, op))
end

local function bLeftReduce(values, op)
  if #values == 1 then
    return values[1]
  end

  if #values == 2 then
    return op(values[1], values[2])
  end

  local middle = math.floor(#values / 2)
  local bottom = slice(values, 1, middle)
  local top = slice(values, middle + 1, #values)
  return op(bRightReduce(bottom, op), bRightReduce(top, op))
end

local function join(values, separator)
  return bRightReduce(values, function (mem, next)
    return mem..separator..next
  end)
end

function Curl:memoizeAssoc(key, left, right, op)
  local entry = function (value, parts)
    return { value = value, parts = parts }
  end

  self[key] = self[key] or {}
  local cache = self[key]

  local lKey   = left:name()
  local lEntry = cache[lKey] or entry(left,  { lKey })

  local rKey   = right:name()
  local rEntry = cache[rKey] or entry(right, { rKey })

  local cParts = append(lEntry.parts, rEntry.parts)
  table.sort(cParts)

  local cKey = join(cParts, "-")
  cache[cKey] = cache[cKey] or entry(op(left, right, cKey), cParts)
  return cache[cKey].value
end

function Curl:sum(left, right)
  return self:memoizeAssoc("sums", left, right, function (left, right, key)
    local sum = self:createObject("Sum", key)
    connect(left,  "Out", sum, "Left")
    connect(right, "Out", sum, "Right")
    return sum
  end)
end

function Curl:mult(left, right)
  return self:memoizeAssoc("mults", left, right, function (left, right, key)
    local mult = self:createObject("Multiply", key)
    connect(left,  "Out", mult, "Left")
    connect(right, "Out", mult, "Right")
    return mult
  end)
end

function Curl:multAll(ins)
  return bLeftReduce(ins, function (mem, next)
    return self:mult(mem, next)
  end)
end

function Curl:logicalOrAll(ins)
  return bLeftReduce(ins, function (mem, next)
    return self:logicalOr(mem, next)
  end)
end

function Curl:hardLimit(input, limit)
  local top    = self:cFold(input, limit, 1, 0)
  local bottom = self:cFold(top, self:negate(limit), 0, 1)
  return bottom
end

function Curl:oneFold(input, low, high)
  local top    = self:cFold(input, high, 1, -1)
  local bottom = self:cFold(top,   low, -1,  1)
  return bottom
end

function Curl:manyFold(input, low, high, depth)
  local fold = input
  for i = 1, depth do
    fold = self:oneFold(fold, low, high)
  end
  return fold
end

function Curl:reflectPoint()
  return self:mult(self:limitControl(), self:reflectControl())
end

function Curl:middleSignal(input)
  local topThreshold = self:pRectify(self:reflectPoint())
  local foldTop      = self:cFold(input, topThreshold, 1, 0)

  local bottomThreshold = self:negate(topThreshold)
  local foldBottom      = self:cFold(foldTop, bottomThreshold, 0, 1)

  return foldBottom
end

function Curl:topSignal(input)
  return self:manyFold(
    self:pRectify(
      self:sum(
        input,
        self:negate(self:pRectify(self:reflectPoint()))
      )
    ),
    self:nRectify(self:reflectPoint()),
    self:mult(
      self:limitControl(),
      self:logicalNot(self:pRectify(self:reflectPoint()))
    ),
    config.depth
  )
end

function Curl:bottomSignal(input)
  return self:negate(
    self:manyFold(
      self:pRectify(
        self:sum(
          self:negate(input),
          self:negate(self:pRectify(self:reflectPoint()))
        )
      ),
      self:nRectify(self:reflectPoint()),
      self:mult(
        self:limitControl(),
        self:logicalNot(self:pRectify(self:reflectPoint()))
      ),
      config.depth
    )
  )
end

function Curl:xFade(a, b, fade)
  local xFade = self:createObject("CrossFade", a:name().."X"..b:name())
  connect(a,    "Out", xFade, "A")
  connect(b,    "Out", xFade, "B")
  connect(fade, "Out", xFade, "Fade")
  return xFade
end

function Curl:sideSignal(input)
  local withGb = self:withGainBias(input)

  local middle = self:middleSignal(withGb)
  local top    = self:topSignal(withGb)
  local bottom = self:bottomSignal(withGb)

  local topBottom  = self:sum(top, bottom)
  
  local filter = self:createObject("LadderFilter")
  connect(topBottom, "Out", filter, "In")
  local wetAdapt = self:adapt(self:wetControl(), 2000, 20)
  tie(filter, "Fundamental", wetAdapt, "Out")
  local full  = self:sum(middle, filter)

  local xFade = self:xFade(full, input, self:wetControl())

  return self:hardLimit(xFade, self:limitControl())
end

function Curl:leftOutputSignal()
  return self:sideSignal(self:leftInputSignal())
end

function Curl:rightOutputSignal()
  return self:sideSignal(self:rightInputSignal())
end

function Curl:onLoadGraph(channelCount)
  local left = self:leftOutputSignal()
  connect(left, "Out", self, "Out1")

  if channelCount > 1 then
    local right = self:rightOutputSignal()
    connect(right, "Out", self, "Out2")
  end
end

function Curl:onLoadViews(objects, branches)
  local controls, views = {}, {
    expanded  = { "gain", "bias", "limit", "reflect", "wet" },
    collapsed = { "wet" },

    gain    = { "wave3", "gain" },
    bais    = { "wave3", "bias" },
    limit   = { "wave3", "limit" },
    reflect = { "wave3", "reflect" },
    wet     = { "wave3", "wet" }
  }

  controls.wave2 = OutputScope {
    monitor = self,
    width   = 2 * ply
  }

  controls.wave3 = OutputScope {
    monitor = self,
    width   = 3 * ply
  }

  controls.gain    = self:gainView()
  controls.bias    = self:biasView()
  controls.limit   = self:limitView()
  controls.reflect = self:reflectView()
  controls.wet     = self:wetView()

  return controls, views
end

return Curl