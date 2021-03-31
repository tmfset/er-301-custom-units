## And

A logical `And` gate.

Output goes high when the **unit input** _and_ the **gate** input are greater than zero.

| Input | Description |
| --- | --- |
| **_unit input_** | The left input to the `and` gate. |
| **gate** | The right input to the `and` gate. |

## Or

A logical `Or` gate.

Output goes high when either the **unit input** _or_ the **gate** input is greater than zero.

| Input | Description |
| --- | --- |
| **_unit input_** | The left input to the `or` gate. |
| **gate** | The right input to the `or` gate. |

## Not

A logical `Not` gate.

Output goes high when the **unit input** is less than or equal to zero.

| Input | Description |
| --- | --- |
| **_unit input_** | The input to the `not` gate. |

## Trig

Convert the input signal to a trigger.

Outputs a trigger when the **unit input** is greater than zero.

| Input | Description |
| --- | --- |
| **_unit input_** | The input to the trigger converter. |

## Latch

An SR Latch.

Output is latched high when the **unit input** is greater than zero.

| Input | Description |
| --- | --- |
| **_unit input_** | The latch is set when the **unit input** is greater than zero. |
| **reset** | Reset the latch to zero on gate high unless the **unit input** is greater than zero. |

## DLatch

A data latch.

Samples the **unit input** when the **clock** gate goes high. Re-samples the **unit input** when **reset** is high.

### Patch Ideas
1. _Sample and Hold_<br><br>Patch a signal into the **clock** input to create a sample and hold function.

2. _Track and Hold_<br><br>Patch a signal into the **clock** input and hold **reset** high to creatte a track and hold function.

3. _Shift Register_<br><br>Patch several `DLatches` in a row and give them all the same **clock** signal. The left-most **unit input** will be shifted through the `DLatches` on each clock tick.

| Input | Description |
| --- | --- |
| **_unit input_** | The input to be sampled. |
| **clock** | Sample the **unit input** on gate high. |
| **reset** | Reset the internal latch to cause the **unit input** to be re-sampled on the clock. |

## Pick

A VC switch.

Output the **unit input** when **pick** is low and the **alt** signal when **pick** is high.

### Patch Ideas
1. _Sequence Swap_<br><br>Pass in two sequences or CV sources and switch between them at will with the **pick** gate.

2. _Audio Rate Slice_<br><br>Rapidly switch between two signals with an audio-rate **pick** input.

| Input | Description |
| --- | --- |
| **_unit input_** | The left input, output when **pick** is low. |
| **alt** | The right input, output when **pick** is high. |
| **pick** | Output the **unit input** when low and **alt** when high. |