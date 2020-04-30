Sloop! :boat:

A **synchronized looper** unit for the ER-301, built and tested on firmware **vx.x.xx**.

I originally prototyped this unit through the UI but quickly discovered I couldn't handle the feature complexity without some serious abstraction. Thankfully the **Middle Layer SDK** allows for exactly that, allowing for a feature rich native looping utility!

Installation

To install, simply download this repo and copy the Sloop directory to your SD card under ER-301/libs.

The new unit will be available on the insert screen under Recording and Loopers.

Patching Ideas

4 Bar Looper - This is the default configuration of the unit. Simply connect a clock that triggers once per bar.

Parameter Overview

clock

The clock input the looper will be synchronized with.

engage

Toggle to engage the looper playback. When the engage latch is toggled, playback will begin on the next clock tick. Similarly, when disabled it will stop on the next clock tick.

The internal looper clocks will only tick when the engage latch is active, so if playback is stopped for a while it will continue from the step it was on before resetting.

reset

Trigger a reset on the next clock tick.

record

Trigger loop recording on the next clock tick. The unit will then record for rSteps beats.

steps

The number of clock ticks before the looper resets from the beginning, i.e. the loop length.

rSteps

The number of clock ticks to keep recording active after it is triggered.

feedback

How much should the already recorded level be reduced when the record head passes over it? With a value of 1, newly recorded data will be layer on top of the existing buffer data. With a value less than one, the data in the buffer will slowly fade out.

fade

How long to fade out the input after the record head finishes. This is intended to smooth out recordings instead of having a hard cut at the end.

Configuration

New... 

Create a new buffer.

Pool...

Select an existing buffer from the pool.

Card...

Load a sample from the SD card and it as the loop buffer.

Edit...

Open the buffer editor to modify it (trim, normalize, etc.)

Detach!

Detach the attached buffer.

Zero!

Zero the attached buffer. Provides a quick way to clear the active loop.

