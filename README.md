# Befaco

Based on [Befaco](http://www.befaco.org/) Eurorack modules.

[VCV Library page](https://library.vcvrack.com/Befaco)

[License](LICENSE.md)


## Differences with hardware

We have tried to make the VCV implementations as authentic as possible, however there are some minor changes that have been made (either for usuability or to bring modules in line with VCV rack conventions and standards).

* Sampling Modulator removes the DC offset of "Clock Out" and "Trigg Out" to allow these to be easily used as oscillators - the legacy behaviour (0 - 10V pulses) can be selected via the context menu.

* The hardware version of Morphader accepts 0-8V CV for the crossfade control, here we widen this to accept 0-10V.

* Chopping Kinky hardward is DC coupled, but we add the option (default disabled) to remove this offset.

* The hardware Muxlicer assigns multiple functions to the "Speed Div/Mult" dial, that cannot be reproduced with a single mouse click. Some of these have been moved to the context menu, specifically: quadratic gates, the "All In" normalled voltage, and the input/output clock division/mult. The "Speed Div/Mult" dial remains only for main clock div/mult.