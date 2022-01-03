local RegisterShared = {}

function RegisterShared.serializeRegister(register)
  local max = register:getMax()
  local data = {}

  for i = 1, max do
    data[i] = register:getData(i - 1);
  end

  return {
    max    = max,
    step   = register:getStep(),
    shift  = register:getShift(),
    length = register:getSeqLength(),
    data   = data
  }
end

function RegisterShared.deserializeRegister(register, t)
  register:setMax(t.max)
  register:setStep(t.step)
  register:setShift(t.shift)
  register:setSeqLength(t.length)

  for i, v in ipairs(t.data) do
    register:setData(i - 1, v)
  end

  register:triggerDeserialize();
end

return RegisterShared
