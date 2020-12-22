## Curl (Wave Folder)

A **wave folder** unit for the ER-301, built and tested on firmware **v0.5.03**.

A unit for adding character to simple waveforms / distortion to more complex signals. What could be more useful than a good ol-fashioned wave folder?

![](../screenshots/CurlSine.png)

### Installation

To install, simply download this repo and copy the `Curl` directory to your SD card under `ER-301/libs`.

The new unit will be available on the insert screen under **Mapping and Control**.

### Parameter Overview

Parameter | Description
--------- | -----------
**gain** | Add gain to the input signal to push it harder against the **limit**, forcing the wave to fold.
**limit** | The peak amplitude where the wave will fold. After reaching the max fold _depth_ the signal will hard clip.
**reflect** | The percentage of the limit that the wave will reflect at. At 0 the wave will fold back at the center. At -1 it will fully invert before folding back. Values near 1 will impart high frequency distortion on the signal.
**wet** | The wet/dry mix. At 1 the wave will be completely affected.
