## Simple Synthesizers

A collection of **simple polyphonic synthesizer** units for the ER-301, built and tested on firmware **v0.4.26**.

### Installation
To install, download this repo and copy the `SimpleSynth` directory to your SD card under `ER-301/libs`.

### Overview

The new units will be available on the insert screen.

![](screenshots/SimpleSynthInsert.png)
![](screenshots/SimpleSynthLoaded.png)

The following parameters are available:

Parameter | Description
--------- | -----------
**gate*N*** | The **gate** input for the **N**th voice. Each voice has it's own ADSR envelope triggered by this gate. Switch this gate to toggle mode to convert the unit into an oscillator bank.
**V/Oct*N*** | The **V/Oct** for the **N**th voice.
**f0** | The **fundamental frequency** for all voices.
**detune** | The **pitch offset** of the second oscillator in all voices. Each voice is comprised of **two saw waves**.
**cutoff** | The **base filter cutoff** for all voices. Each voice has it's own **low-pass ladder filter**.
**Q** | The **filter resonance** for all voices. Starts to self resonate around 0.6.
**fenv** | The **filter envelope amount** for all voices. Determines how much the ADSR for each voice affects the filter **cutoff**. Can also be set to negative values to "duck" the filter.
**A** | The ADSR **attack** value for all voices.
**D** | The ADSR **decay** value for all voices.
**S** | The ADSR **sustain** level for all voices. Can also be used as an overall level control in certain situations.
**R** | The ADSR **release** level for all voices.
