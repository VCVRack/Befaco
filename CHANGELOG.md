# Change Log

## v2.8.1
  * Noise Plethora
    * Fix bug where program choice is wrongly copied between top and bottom sections

## v2.8.0
  * Molten Bypass
    * Initial release
  * EvenVCO
    * Complete re-write for better FM performance
    * Hard sync added
  * Octaves 
    * Avoid allocation in the audio thread (thanks @danngreen)
  * Noise Plethora
    * Fix labels
    * Avoid std::string allocations on audio thread (thanks @danngreen)
  
## v2.7.1
  * Midi Thing 2
    * Remove -10 to 0 V configuration
    * Add menu option to set all channels mode at once
    * Fix appearance in module browser

## v2.7.0
  * Midi Thing 2
    * Initial release
  * Octaves
    * Better default oversampling setting (x4)


## v2.6.0
  * Octaves 
    * Initial release
  * Misc
    * Better default values for ADSR and Burst


## v2.5.0
  * Burst
    * Initial release
  * Voltio
    * Initial release
  * PonyVCO
    * Now polyphonic
  * Misc
    * Fix trigger inputs to follow Rack voltage standards (Kickall, Muxlicer, Rampage)

## v2.4.1
  * Rampage
    * Fix SIMD bug

## v2.4.0
  * MotionMTR
    * Initial release
  * Muxlicer
    * Fix gate labels
    * Improve docs
  * PonyVCO
    * Fix NaNs when TZFM input is used at very high sample rates
    * Fix pitch wobble when disconnecting TZFM voltage source

## v2.3.0
  * PonyVCO
    * Initial release
  * EvenVCO
    * Optionally remove DC from pulse wave output
  * StereoStrip
    * Address high CPU usage when using EQ sliders

## v2.2.0

  * StereoStrip
    * Initial release

## v2.1.1
  * Noise Plethora
    * Grit quantity knob behaviour updated to match production hardware version

## v2.1.0
  * Noise Plethora
    * Initial release
  * Chopping Kinky
    * Upgraded to use improved DC blocker
  * Spring Reverb
    * Added bypass
  * Kickall
    * Allow trigger input and button to work independently
  * EvenVCO
    * Fix to remove pop when number of polyphony engines changes
  * Muxlicer
    * Chaining using reset now works correctly

## v2.0.0
  * update to Rack 2 API (added tooltips, bypass, removed boilerplate etc)
  * UI overhaul

## v1.2.0

  * Released new modules: Muxlicer, Mex, Morphader, VC ADSR, Sampling Modulator, ST Mix
  * Removed DC offset from EvenVCO pulse output

## v1.1.1

  * Fixed issue with A*B+C summing logic

## v1.1.0

  * New modules added: Kickall, Percall, Chopping Kinky, and Hexmix VCA
  * Added polyphony support for some modules

## v1.0.0

  * Initial set of modules: EvenVCO, Rampage, A*B+C, Spring Reverb, Mixer, Slew Limiter, Dual Atenuverter