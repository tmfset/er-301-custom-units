## Strike LPG

A **low pass gate** unit for the ER-301, built and tested on firmware **v0.5.01**.

I frequently find myself tying together an envelope, filter, and vca to control incoming audio on the ER-301 so I created this LPG to take care of it in one compact unit!

### Installation

To install, simply download this repo and copy the `Strike` directory to your SD card under `ER-301/libs`.

The new unit will be available on the insert screen under **Filtering**.

### Parameter Overview

Parameter | Description
--------- | -----------
**strike** | Use a gate or trigger to "**strike**" the LPG. This does not model a vactrol or anything fancy like that, just an exponential envelope on the filter cutoff and vca level.
**loop** | Toggle to automatically re-**strike** the LPG when the envelope closes (EOF, "end of fall"). Use in conjuction with the **strike** input to create weird rythms.
**lift** | At 0 the filter remains completely closed. At 1 the filter will open to the **peak** frequency.
**peak** | The maximum frequency the filter will open to when **lift** is 1.
**Q** | The filter resonance, simple as that.
**time** | A multiplier for the **attack** and **decay** time.
**attack** | The time it takes for the filter and vca to completely open after a **strike**.
**decay** | The time it takes for the filter and vca to completely close after the **attack** time has elapsed.
**curve** | A multiplier for the **curveIn** and **curveOut** values.
**curveIn** | How much of the _logarithmic_ **attack** curve to use. This is mixed in using the built-in envelope follower DSP.
**curveOut** | How much of the _exponential_ **decay** curve to use. This is mixed in using the built-in envelope follower DSP.
