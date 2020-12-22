## Arc (Envelope)

An **AR Envelope** unit for the ER-301, built and tested on firmware **v0.5.03**.

This is just the envelope from [Strike](../Strike/README.md) extracted as it's own unit.

### Installation

To install, simply download this repo and copy the `Arc` directory to your SD card under `ER-301/libs`.

The new unit will be available on the insert screen under **Envelopes**.

### Parameter Overview

Parameter | Description
--------- | -----------
**strike** | Use a gate or trigger to initate the arc.
**loop** | Toggle to automatically re-**strike** when the envelope closes (EOF, "end of fall"). Use in conjuction with the **strike** input to create funky rythms.
**lift** | How high to arc to.
**time** | A multiplier for the **attack** and **decay** time.
**attack** | The time it takes for the arc to completely open after a **strike**.
**decay** | The time it takes for the arc to completely close after the **attack** time has elapsed.
**curve** | A multiplier for the **curveIn** and **curveOut** values.
**curveIn** | How much of the _logarithmic_ **attack** curve to use. This is mixed in using the built-in envelope follower DSP.
**curveOut** | How much of the _exponential_ **decay** curve to use. This is mixed in using the built-in envelope follower DSP.
