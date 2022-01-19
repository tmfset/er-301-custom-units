local common = require "common.lib"

local function linearMap(min, max, zero, rounding, wrap)
  return function (sc, c, f, sf)
    return common.LinearDialMap(min, max, zero, rounding, wrap or false, sc, c, f, sf)
  end
end

local with = {
  unit  = linearMap(-1, 1, 0, 0.001),
  punit = linearMap( 0, 1, 0, 0.001),
  count = {
    zeroTo = function (max, zero, wrap)
      return linearMap(0, max, zero or 0, 1, wrap)
    end,
    oneTo = function (max, zero, wrap)
      return linearMap(1, max, zero or 1, 1, wrap)
    end,
    span = function (max, zero, wrap)
      return linearMap(-max, max, zero or 0, 1, wrap)
    end
  },
  cents = {
    threeOctaves = linearMap(-3600, 3600, 0, 1)
  }
}

return {
  unit = {
    with    = with.unit,
    default = with.unit(0.1, 0.01, 0.005, 0.001)
  },
  punit = {
    with    = with.punit,
    default = with.punit(0.1, 0.01, 0.005, 0.001)
  },
  count = {
    zeroTo = with.count.zeroTo,
    oneTo  = with.count.oneTo,
    span   = with.count.span
  },
  cents = {
    threeOctaves = with.cents.threeOctaves(1200, 100, 10, 1)
  }
}
