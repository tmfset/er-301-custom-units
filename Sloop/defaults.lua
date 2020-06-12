-- A default configuration for Sloop to start in manual mode.
local defaultManual = {
  maxSteps                = 64,
  initialSteps            = 4,
  initialFeedback         = 1,
  startEngaged            = 1, -- yes
  defaultMode             = 2, -- manual
  defaultLengthMode       = 2, -- free
  defaultResetOnDisengage = 2, -- no
  defaultResetOnRecord    = 2  -- no
}

-- A default configuration for Sloop to start in continuous mode.
local defaultContinuous = {
  maxSteps                = 64,
  initialSteps            = 4,
  initialFeedback         = 1,
  initialFadeIn           = 0.01,
  initialFadeOut          = 0.1,
  initialThrough          = 1,

  startEngaged            = 1, -- yes
  defaultMode             = 1, -- continuous
  defaultLengthMode       = 1, -- locked
  defaultResetOnDisengage = 2, -- no
  defaultResetOnRecord    = 2  -- no
}

return defaultContinuous