# Befaco

Based on [Befaco](http://www.befaco.org/) Eurorack modules.

[VCV Library page](https://library.vcvrack.com/Befaco)

[License](LICENSE.md)


## Differences with hardware

We have tried to make the VCV implementations as authentic as possible, however there are some minor changes that have been made (either for usuability or to bring modules in line with VCV rack conventions and standards).

* Sampling Modulator removes the DC offset of "Clock Out" and "Trigg Out" to allow these to be easily used as oscillators - the legacy behaviour (0 - 10V pulses) can be selected via the context menu.

* The hardware version of Morphader accepts 0-8V CV for the crossfade control, here we widen this to accept 0-10V.

* Chopping Kinky hardward is DC coupled, but we add the option (default disabled) to remove this offset.

* See [docs/Muxlicer.md](docs/Muxlicer.md)

* The Noise Plethora filters self-oscillate on the hardware version but not the software version. 

* EvenVCO has the option (default true) to remove DC from the pulse waveform output (hardware contains DC for non-50% duty cycles).

* PonyVCO optionally allows the user:
  * to filter DC from the TZFM input signal (hardware filters below 15mHz)
  * to limit the pulsewidth from 5% to 95% (hardware is full range)
  * to remove DC from the pulse waveform output (hardware contains DC for non-50% duty cycles)

* MotionMTR optionally doesn't use the 10V normalling on inputs if in audio mode to avoid acidentally adding unwanted DC to audio signals, see context menu. E.g. if you temporarily unpatch an audio source whilst using it it mixer mode, you get 10V DC suddenly and a nasty pop.

* Burst hardware version version can also set the tempo by tapping the encoder, this is not possible in the VCV version. 