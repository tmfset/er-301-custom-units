import sys
import wave
import struct
from math import exp

SAMPLERATE = 48000
SCALE = 65535 / 2.0

def calc_exp(x, rise):
    divisor = float(exp(rise) - 1)
    numerator = float(rise * exp(x * rise))
    return ((numerator / divisor) - (rise / divisor)) / rise

def calc_log(x, rise):
    exp = calc_exp(x, rise)
    return -1 * exp + 1

def wav_data(resolution, rise, scale):
    data = []
    half_resolution = resolution / 2
    for frame in range(half_resolution):
        val = struct.pack('h', calc_exp(frame / float(half_resolution), rise) * scale)
        data.append(val)
    for frame in range(half_resolution):
        val = struct.pack('h', calc_log(frame / float(half_resolution), rise) * scale)
        data.append(val)
    return data

def filename(rise):
    return 'exp-%s.wav'%(rise)

def write_wav(resolution, rise, rate, scale,):
    data = wav_data(resolution, rise, scale)

    file = wave.open(filename(rise), 'w')
    file.setparams((1, 2, rate, 0, 'NONE', 'not compressed'))
    file.writeframes(b''.join(data))
    file.close()

if __name__ == '__main__':
    rise = float(sys.argv[1])
    write_wav(SAMPLERATE, rise, SAMPLERATE, SCALE)
