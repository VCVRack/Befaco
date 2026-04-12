# Noise Plethora — Complete Plugin Guide

*Befaco Noise Plethora v1.5 + Banks D, E & F Extension*

## About This Guide

The Noise Plethora is a Eurorack noise workstation with 2 digital sound generators (A and B), each followed by an analog multimode filter. Generators A and B run interchangeable algorithms organized in banks of 10. Each algorithm has two parameters controlled by the **X** and **Y** knobs (and their corresponding CV inputs, 0–10Vpp).

This guide documents all 60 algorithms across 6 banks:

| Bank | Name | Programs | Theme |
|------|------|----------|-------|
| **A** | Textures | 0–9 | Cross-modulation, ring mod, granular, noise AM |
| **B** | HH Clusters | 0–9 | Additive/cluster synthesis with 6–16 oscillators |
| **C** | Harsh & Wild | 0–9 | Bitcrushing, random walks, chaotic oscillators |
| **D** | Resonant Bodies | 0–9 | Delay-based resonance, physical modeling, spectral shaping |
| **E** | Chaos Machines | 0–9 | Deterministic chaos, algorithmic processes, digital manipulation |
| **F** | Stochastic | 0–9 | Random pulses, metallic noise, LFSR sequences, noise textures |

Banks A–C are the original Befaco firmware. Banks D–F are community extensions.

---

## Table of Contents

- [Bank A: Textures](#bank-a-textures)
  - [A-0: RadioOhNo](#a-0-radioohno) — Cross-modulated square waves
  - [A-1: Rwalk_SineFMFlange](#a-1-rwalk_sinefmflange) — Random walk FM + flanger
  - [A-2: xModRingSqr](#a-2-xmodringsqr) — Cross FM square ring mod
  - [A-3: XModRingSine](#a-3-xmodringsine) — Cross FM sine ring mod
  - [A-4: CrossModRing](#a-4-crossmodring) — 4-osc cascaded ring mod
  - [A-5: Resonoise](#a-5-resonoise) — Wavefolder-controlled resonant filter
  - [A-6: GrainGlitch](#a-6-grainglitch) — Granular + XOR
  - [A-7: GrainGlitchII](#a-7-grainglitchii) — Granular amplified
  - [A-8: GrainGlitchIII](#a-8-grainglitchiii) — Granular sawtooth
  - [A-9: Basurilla](#a-9-basurilla) — Noise gated by 3 pulse LFOs
- [Bank B: HH Clusters](#bank-b-hh-clusters)
  - [B-0: ClusterSaw](#b-0-clustersaw) — 16 sawtooths, geometric spread
  - [B-1: PwCluster](#b-1-pwcluster) — 6 pulse waves, adjustable PW
  - [B-2: CrCluster2](#b-2-crcluster2) — 6 sines, single FM modulator
  - [B-3: SineFMcluster](#b-3-sinefmcluster) — 6 triangles, independent sub-FM
  - [B-4: TriFMcluster](#b-4-trifmcluster) — 6 triangles, ultra-slow FM
  - [B-5: PrimeCluster](#b-5-primecluster) — 16 oscillators at prime frequencies
  - [B-6: PrimeCnoise](#b-6-primecnoise) — Primes, wider range variant
  - [B-7: FibonacciCluster](#b-7-fibonaccicluster) — 16 sawtooths, Fibonacci spacing
  - [B-8: PartialCluster](#b-8-partialcluster) — 16 sawtooths, multiplicative spread
  - [B-9: PhasingCluster](#b-9-phasingcluster) — 16 squares with 16 independent LFOs
- [Bank C: Harsh & Wild](#bank-c-harsh--wild)
  - [C-0: BasuraTotal](#c-0-basuratotal) — LFSR-triggered oscillator + reverb
  - [C-1: Atari](#c-1-atari) — 8-bit cross-modulation
  - [C-2: WalkingFilomena](#c-2-walkingfilomena) — 16 random-walk oscillators
  - [C-3: S_H](#c-3-s_h-sample--hold) — Sample & hold + reverb
  - [C-4: ArrayOnTheRocks](#c-4-arrayontherocks) — Randomized wavetable FM
  - [C-5: ExistenceIsPain](#c-5-existenceispain) — 4 LFO-swept bandpass filters
  - [C-6: WhoKnows](#c-6-whoknows) — Fast LFO bandpass filters
  - [C-7: SatanWorkout](#c-7-satanworkout) — Pink noise FM + reverb
  - [C-8: Rwalk_BitCrushPW](#c-8-rwalk_bitcrushpw) — 9 random walks, 1-bit crush
  - [C-9: Rwalk_LFree](#c-9-rwalk_lfree) — 4 PWM random walks + reverb
- [Bank D: Resonant Bodies](#bank-d-resonant-bodies)
  - [D-0: CombNoise](#d-0-combnoise) — 4 parallel comb filters
  - [D-1: PluckCloud](#d-1-pluckcloud) — 6 Karplus-Strong voices
  - [D-2: TubeResonance](#d-2-tuberesonance) — Harmonic comb filter tube
  - [D-3: FormantNoise](#d-3-formantnoise) — Vowel formant filters
  - [D-4: BowedMetal](#d-4-bowedmetal) — 8 metallic resonators
  - [D-5: FlangeNoise](#d-5-flangenoise) — Pink noise flanger
  - [D-6: NoiseBells](#d-6-noisebells) — 4 sharp bell resonators
  - [D-7: NoiseHarmonics](#d-7-noiseharmonics) — Wavefolder + bandpass
  - [D-8: IceRain](#d-8-icerain) — S&H droplets + reverb
  - [D-9: DroneBody](#d-9-dronebody) — Cross-FM drone + wavefolder
- [Bank E: Chaos Machines](#bank-e-chaos-machines)
  - [E-0: LogisticNoise](#e-0-logisticnoise) — Logistic map chaos
  - [E-1: HenonDust](#e-1-henondust) — Henon attractor crackle
  - [E-2: CellularSynth](#e-2-cellularsynth) — Cellular automaton + 8 oscillators
  - [E-3: StochasticPulse](#e-3-stochasticpulse) — Random impulses + resonant filter
  - [E-4: BitShift](#e-4-bitshift) — Two sawtooths XOR'd
  - [E-5: FeedbackFM](#e-5-feedbackfm) — Self-feedback FM sine
  - [E-6: RunawayFilter](#e-6-runawayfilter) — Self-oscillating SVF
  - [E-7: GlitchLoop](#e-7-glitchloop) — Loop with bit degradation
  - [E-8: SubHarmonic](#e-8-subharmonic) — Frequency dividers
  - [E-9: DualAttractor](#e-9-dualattractor) — Coupled Lorenz oscillators
- [Bank F: Stochastic](#bank-f-stochastic)
  - [F-0: PulseWander](#f-0-pulsewander) — Smooth random walk pulses
  - [F-1: TwinPulse](#f-1-twinpulse) — Two correlated random walks
  - [F-2: QuantPulse](#f-2-quantpulse) — Quantized random (1-6 bits)
  - [F-3: ShapedPulse](#f-3-shapedpulse) — Shaped probability distribution
  - [F-4: ShiftPulse](#f-4-shiftpulse) — 4-stage shift register sequences
  - [F-5: MetallicNoise](#f-5-metallicnoise) — 6 inharmonic square oscillators
  - [F-6: NoiseSlew](#f-6-noiseslew) — Noise with adjustable LP slew
  - [F-7: NoiseBurst](#f-7-noiseburst) — Sporadic noise bursts
  - [F-8: LFSRNoise](#f-8-lfsrnoise) — LFSR pseudo-random sequences
  - [F-9: DualPulse](#f-9-dualpulse) — Two S&H at independent rates

---

## Bank A: Textures

Cross-modulation, ring modulation, granular processing, and noise AM. Focused on complex modulation techniques that create rich, evolving textures.

---

### A-0: RadioOhNo

**Four square wave oscillators cross-modulated in couples, summed together.**

Four pulse-width-modulated oscillators interconnected in feedback pairs: oscillator 1 modulates 2 and vice versa, oscillator 3 modulates 4 and vice versa. A DC source modulates all oscillators' frequency. The cross-modulation creates complex sidebands and chaotic harmonic interactions.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Oscillator 1 frequency | 20 – 2520 Hz | Base frequency for all 4 oscillators (each at a different ratio). Quadratic mapping for fine control at low frequencies. |
| **Y (k2)** | PWM / DC modulation | 0 – 1 | Controls DC amplitude which modulates all oscillators' frequency. Higher values create wider FM deviation and more chaotic sidebands. |

**Sound character:** Aggressive, unstable radio-frequency-like textures. Like tuning between shortwave stations. Rich in aliasing and intermodulation products.

---

### A-1: Rwalk_SineFMFlange

**Four random walkers controlling pulse oscillators, FM'd by sines, through a flanger.**

Four 2D random walkers move in a bounded box with random velocities. Their positions map to the frequencies of 4 pulse wave oscillators, which feed into 4 sine FM oscillators. The mixed output passes through a flanger effect.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | FM sine frequency | 10 – 510 Hz | Base frequency of the 4 sine FM oscillators (each offset by 55–75 Hz). |
| **Y (k2)** | Flanger modulation rate | 0 – 3 Hz | LFO speed of the flanger. Adds swept comb filtering on top of the FM textures. |

**Sound character:** Morphing, evolving FM textures with organic movement. The flanger adds spatial depth and sweeping spectral animation. No two moments are alike. Atmospheric and complex.

---

### A-2: xModRingSqr

**Cross FM between two square wave oscillators, ring modulated.**

Two square wave oscillators in a feedback configuration: each modulates the other's frequency. Their outputs are ring modulated (multiplied) together.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Oscillator 1 frequency | 100 – 5100 Hz | Pitch of the first square wave. Quadratic mapping. |
| **Y (k2)** | Oscillator 2 frequency | 20 – 1020 Hz | Pitch of the second square wave. Quadratic mapping. |

**Sound character:** Metallic, clanging ring modulation. At simple ratios (2:1, 3:2) = tonal metallic tones. At complex ratios = inharmonic metallic clatter.

---

### A-3: XModRingSine

**Cross FM between two sine oscillators, ring modulated.**

Same topology as xModRingSqr but using sine FM oscillators. Sines produce cleaner, more bell-like results than squares.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Oscillator 1 frequency | 100 – 8100 Hz | Pitch of the first sine. Quadratic mapping. Wide range. |
| **Y (k2)** | Oscillator 2 frequency | 60 – 3060 Hz | Pitch of the second sine. Quadratic mapping. |

**Sound character:** Bell-like and metallic tones. All spectral content comes from the FM and ring modulation. Good for tuned metallic percussion sounds.

---

### A-4: CrossModRing

**Four oscillators in a cross-modulation matrix with cascaded ring modulators.**

Four oscillators (2 square, 1 sawtooth with offset, 1 square) modulate each other's frequencies. Three ring modulators in a cascaded configuration: multiply1 and multiply2 feed into multiply3. The most complex modulation topology in the module.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Frequency (all oscillators) | 1 – 827 Hz | Scales all 4 oscillator frequencies with individual ratios. |
| **Y (k2)** | FM depth | 2 – 10 octaves | Master frequency modulation depth. Low = subtle. High = total harmonic chaos. |

**Sound character:** Incredibly dense sideband spectrum. From metallic drones at low FM depth to total chaos at high depth. Like a 4-operator FM synth pushed to extremes.

---

### A-5: Resonoise

**Square wave FM-modulating a sine, through a wavefolder, controlling a resonant filter on white noise.**

A modulated square wave drives a sine FM oscillator. The sine passes through a wavefolder, then feeds into a state-variable filter as the frequency control signal. White noise is the audio input to the filter (resonance=3).

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Both oscillator frequencies | 20 – 10020 Hz | Controls the modulation rate of the filter cutoff. |
| **Y (k2)** | Wavefold amount | 0.03 – 0.23 | DC bias into the wavefolder. More folding = more complex filter modulation pattern. |

**Sound character:** Resonant noise with animated filter modulation. Like white noise through an auto-wah driven by a complex LFO. Organic and vocal.

---

### A-6: GrainGlitch

**Square wave through a granular cell, output XOR'd with the input.**

A square wave feeds into a granular pitch-shifting processor with feedback. The original square wave and the granular output are combined using XOR digital logic.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Square wave frequency | 500 – 5500 Hz | Base frequency of the source oscillator. |
| **Y (k2)** | Grain size + speed | 25–100 ms / 0.125–8x | Grain size (pitch shift window) and playback speed ratio. |

**Sound character:** Glitchy digital artifacts from XOR combination. Harsh, broken, digital — like a CD skipping.

---

### A-7: GrainGlitchII

**Square wave through a granular cell, amplified.**

Similar to GrainGlitch but without XOR. Granular output amplified 32000x to bring up the quiet signal.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Square wave frequency | 500 – 5500 Hz | Base frequency of the source oscillator. |
| **Y (k2)** | Grain size + speed | 25–100 ms / 0.125–8x | Same mapping as GrainGlitch. |

**Sound character:** Cleaner granular texture than GrainGlitch. More transparent and "musical" than the XOR version.

---

### A-8: GrainGlitchIII

**Sawtooth wave through a granular cell.**

Similar to GrainGlitchII but using a sawtooth waveform. Richer harmonic spectrum provides more material for the granular processor.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Sawtooth frequency | 400 – 5500 Hz | Base frequency. |
| **Y (k2)** | Grain size + speed | 25–80 ms / 0.125–8x | Slightly narrower grain range than I/II. |

**Sound character:** The richest granular variant. Textures from buzzy granular drones to scattered metallic fragments.

---

### A-9: Basurilla

**White noise amplitude-modulated by 3 independent pulse wave LFOs.**

Three parallel channels: white noise is ring-modulated by 3 pulse wave oscillators at different rates and pulse widths. The pulse waves gate the noise in complex rhythmic patterns.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | LFO frequencies | 0.1 – 110 Hz | Controls all 3 pulse LFOs. Low = slow gates. High = audio-rate AM. |
| **Y (k2)** | Pulse widths / noise level | PW + inverse amplitude | Pulse width of the 3 LFOs and inversely controls noise amplitude. |

**Sound character:** Chaotic, granular-like texture from noise gated by 3 independent pulse trains. "Basurilla" = "little garbage" in Spanish — beautifully organized trash.

---

## Bank B: HH Clusters

Cluster/additive synthesis using 6–16 oscillators at mathematically related frequency ratios. Dense, shimmering tonal textures.

---

### B-0: ClusterSaw

**16 sawtooth oscillators with adjustable geometric frequency spread.**

Sixteen sawtooths tuned in geometric progression. Each frequency is the previous one multiplied by a constant factor.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Base frequency | 20 – 1020 Hz | Fundamental frequency. |
| **Y (k2)** | Spread factor | 1.01 – 1.91 | Ratio between adjacent oscillators. Low = beating unison. High = wide harmonic spread. |

**Sound character:** Shimmering, buzzing mass of sawtooths. The quintessential "cluster" sound. 16 voices create an extremely rich spectral texture.

---

### B-1: PwCluster

**6 detuned pulse waveforms with adjustable pulse width.**

Six pulse waves at fixed detuned ratios (1.227, 1.24, 1.17, 1.2, 1.3). DC-controlled pulse width.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Base frequency | 40 – 8040 Hz | Widest range of any cluster plugin. |
| **Y (k2)** | Pulse width | Wide – Narrow (inverted) | Low Y = full, warm. High Y = thin, nasal. |

**Sound character:** Chorused pulse waves. At wide PW: organ-like. At narrow PW: reedy, nasal. Constant beating from non-harmonic detuning.

---

### B-2: CrCluster2

**6 detuned sine waves with low-frequency FM from a single modulator.**

Six sines, all FM'd by one sine at 2.7x the base frequency.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Base frequency | 40 – 8040 Hz | Cluster fundamental. |
| **Y (k2)** | FM index | 0 – 1.0 | Low = pure sine cluster. High = bright FM harmonics. |

**Sound character:** Clean sine cluster that becomes progressively FM-modulated. Warm to bright. Spectrally coherent.

---

### B-3: SineFMcluster

**6 triangle waves, each independently FM'd by its own sine modulator (subharmonic ratio 0.333).**

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Base frequency | 300 – 8300 Hz | Higher starting frequency. |
| **Y (k2)** | FM index | 0.1 – 1.0 | Subharmonic FM adds content below carriers. |

**Sound character:** Warm, soft FM cluster. Triangle carriers + subharmonic FM = "analog" feeling warmth and depth.

---

### B-4: TriFMcluster

**6 triangle waves, each independently FM'd at very slow ratio (0.07).**

Same architecture as SineFMcluster but with 1/14th FM ratio. Creates phasing/chorus rather than FM timbral change.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Base frequency | 300 – 8300 Hz | Same range as SineFMcluster. |
| **Y (k2)** | FM index | 0.1 – 1.0 | Creates slow drifting detuning, not bright harmonics. |

**Sound character:** Lush, animated ensemble effect. Like 6 slightly out-of-tune instruments. Very different from SineFMcluster despite identical architecture.

---

### B-5: PrimeCluster

**16 oscillators tuned to prime number frequencies, noise-modulated.**

Sixteen variable-triangle oscillators at primes (53, 127, 199... 1523 Hz), all scaled by a common factor. White noise FM adds animation.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Pitch multiplier | 0.5 – 10.5 | Scales all 16 primes. Linear mapping. |
| **Y (k2)** | Noise modulation | 0 – 0.2 | Noise FM amount. Low = clean. High = tremolo-like. |

**Gain:** 0.8 (reduced to prevent clipping).

**Sound character:** Inharmonic, bell-like chord. No common harmonics — a unique shimmering metallic texture.

---

### B-6: PrimeCnoise

**16 triangle oscillators at prime frequencies (wider range variant).**

Nearly identical to PrimeCluster but with quadratic pitch mapping (k1^2 x 12) for wider range.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Pitch multiplier | 0.5 – 12.5 | Quadratic mapping — more extreme range. |
| **Y (k2)** | Noise modulation | 0 – 0.2 | Same as PrimeCluster. |

**Gain:** 0.8

**Sound character:** Same prime character, different response to knob gestures. More exponential/extreme than PrimeCluster.

---

### B-7: FibonacciCluster

**16 sawtooth oscillators at Fibonacci-series frequency spacing.**

Frequencies follow `f(n) = f(n-1) + f(n-2) x spread`. Noise FM adds animation.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Base frequency | 50 – 1050 Hz | Fundamental pitch. |
| **Y (k2)** | Spread factor | 0.1 – 0.6 | How fast frequencies diverge. Near 0.618 = golden ratio intervals. |

**Sound character:** Nature's favorite ratio. At spread near golden ratio, intervals feel "naturally balanced." Unique organic quality.

---

### B-8: PartialCluster

**16 sawtooth oscillators with multiplicative harmonic spacing.**

Each frequency is the previous multiplied by a spread factor.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Base frequency | 50 – 1050 Hz | Fundamental. |
| **Y (k2)** | Harmonic spread | 1.01 – 2.11 | At 2.0 = octave spacing. Below = dense. Above = stretched/metallic. |

**Sound character:** The most "tuneable" cluster. Morph from dense unison to harmonic series to metallic bells by sweeping Y.

---

### B-9: PhasingCluster

**16 square waves with individual LFO phase modulation.**

Each oscillator has its own triangle LFO at a unique rate: 10, 11, 15, 1, 1, 3, 17, 14, 0.11, 5, 2, 7, 1, 0.1, 0.7, 0.5 Hz.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Carrier frequency | 30 – 5030 Hz | Base frequency for all 16 squares. |
| **Y (k2)** | Spread | 1.0 – 1.5 | Spreads carrier frequencies slightly. |

**Sound character:** Massive phasing. 16 oscillators drifting at their own rates — constantly evolving, never-repeating interference patterns. Glacial slow LFOs (0.1 Hz) + shimmer (17 Hz).

---

## Bank C: Harsh & Wild

Extreme processing: bitcrushing, random walks, chaotic oscillators, reverb abuse. The most experimental and aggressive bank.

---

### C-0: BasuraTotal

**"Bent" LFSR driving an oscillator with reverb.**

A Galois LFSR generates pseudo-random timing events that trigger a waveform oscillator. Output through Freeverb.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Oscillator frequency | 200 – 5200 Hz | Pitch of triggered waveform. |
| **Y (k2)** | Event rate | Slow – Fast | Timing between LFSR-triggered events. |

**Sound character:** Sporadic bursts with reverb tails. "BasuraTotal" = "Total Garbage" — chaotic, messy, and beautiful.

---

### C-1: Atari

**Two square waves with PWM/FM cross-modulation.**

First oscillator does PWM on the second, second does FM on the first.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Wave 1 frequency | 10 – 60 Hz | Sub-bass territory. |
| **Y (k2)** | Wave 2 freq + FM depth | 10–210 Hz / 3–11 oct | Higher pitch AND more extreme modulation. |

**Sound character:** Classic 8-bit Atari/chiptune. Buzzy, aggressive, nostalgic.

---

### C-2: WalkingFilomena

**16 random walkers controlling pulse oscillator frequencies.**

Sixteen pulse oscillators with independent 2D random walkers in a bounded box.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Random walk box size | 200 – 1800 Hz | Frequency range of all 16 oscillators. |
| **Y (k2)** | Pulse width | 0.1 – 0.9 | Thin = bright/nasal. Wide = warm/full. |

**Sound character:** Shimmering, constantly evolving polyphonic texture. 16 oscillators wandering in pitch space. Organic and alive.

---

### C-3: S_H (Sample & Hold)

**Sample-and-hold noise with dirty reverb.**

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | S&H rate | 15 – 5015 Hz | Slow = stepped tones. Fast = granular noise. |
| **Y (k2)** | Reverb mix | Dry – Wet | Dry = stochastic clicks. Wet = ambient wash. |

**Sound character:** Classic S&H randomness with spatial depth from reverb.

---

### C-4: ArrayOnTheRocks

**FM synthesis with a randomized 256-point wavetable.**

A 500 Hz sine modulates an arbitrary waveform with randomly corrupted table values.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Wavetable oscillator freq | 10 – 10010 Hz | Playback speed. |
| **Y (k2)** | FM modulator amplitude | 0 – 1.0 | Low = clean wavetable. High = rich FM sidebands. |

**Sound character:** Unpredictable timbral textures from the corrupted wavetable. Different from standard FM because the carrier waveform itself is partially random.

---

### C-5: ExistenceIsPain

**Sample-and-hold noise through 4 bandpass filters modulated by 4 LFOs.**

S&H source into 4 parallel SVF bandpass filters with independent triangle LFOs at 11, 70, 23, 0.01 Hz. Resonance=5.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | S&H noise frequency | 50 – 5050 Hz | Base texture density. |
| **Y (k2)** | Filter sweep range | 0.3 – 3.3 octaves | Narrow = focused. Wide = dramatic animation. |

**Sound character:** Slowly breathing, organic multiband filtering. The 0.01 Hz LFO creates glacial changes, 70 Hz adds shimmer. Deep, meditative, slightly ominous.

---

### C-6: WhoKnows

**Pulse wave through 4 bandpass filters with fast LFO modulation.**

Similar to ExistenceIsPain but with 5 Hz pulse source and much faster LFOs (21, 70, 90, 77 Hz). Resonance=7.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Pulse frequency | 15 – 515 Hz | Source frequency. |
| **Y (k2)** | Filter sweep range | 0.3 – 6.3 octaves | At high settings, filters sweep wildly. |

**Sound character:** Dramatic, rapidly animated filter textures. Audio-rate LFOs create sideband-like effects. Aggressive and chaotic.

---

### C-7: SatanWorkout

**Pink noise FM-ing a PWM oscillator, through reverb.**

Pink noise modulates a PWM oscillator. Freeverb with negative damping (-5).

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | PWM frequency | 8 – 6008 Hz | "Pitch" of noise modulation. |
| **Y (k2)** | Reverb room size | 0.001 – 4.0 | Beyond 1.0 = self-exciting reverb feedback. |

**Sound character:** Raw organic noise with reverb. At high Y the reverb self-excites. Relentless intensity.

---

### C-8: Rwalk_BitCrushPW

**9 random-walk pulse oscillators, bitcrushed to 1 bit, with reverb.**

Nine pulse oscillators with 2D random walkers. Mixed, 1-bit crushed, parallel reverb path.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Pulse width | 0.2 – 0.75 | Timbre control alongside random walk frequencies. |
| **Y (k2)** | Reverb room size | 0 – 4.0 | High = self-exciting reverb. |

**Sound character:** Extreme digital destruction. Dense, distorted digital texture with organic pitch movement from random walks. Lo-fi at its most extreme.

---

### C-9: Rwalk_LFree

**4 PWM oscillators with random-walk frequencies, through reverb.**

Four PWM oscillators with independent random walkers. Clean Freeverb (no bitcrushing).

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Random walk boundary | 50 – 550 Hz | Frequency space size. |
| **Y (k2)** | Reverb room size | 0 – 1.0 | Conservative range, clean reverb. |

**Sound character:** Spacious, clean random pulse textures. Warm timbre, gentle pitch drift, dreamy ambient reverb. The gentlest of the random walk family.

---

## Bank D: Resonant Bodies

Delay-based resonance, physical modeling, and spectral shaping. What happens when noise passes through resonant structures: comb filters, waveguides, feedback networks, and high-Q resonators.

---

### D-0: CombNoise

**White noise through 4 parallel comb filters.**

A comb filter is a short delay line with feedback — it reinforces frequencies at the delay time and its harmonics. Four combs at ratios 1.0 : 0.7 : 0.5 : 0.35 create a rich, multi-pitched metallic tone.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Resonant pitch | 40 – 5000 Hz | Fundamental frequency of all 4 comb filters. |
| **Y (k2)** | Feedback | 0 – 95% | Resonance sustain. Low = colored noise. High = clearly pitched ringing. |

**Sound character:** Like blowing across a metal pipe. The 4 overlapping combs create a rich harmonic spectrum.

---

### D-1: PluckCloud

**6 Karplus-Strong plucked string voices with random retriggering.**

Noise burst into delay line with LP filter in feedback loop. Voices retrigger randomly (~2% per cycle). Detuned at ratios 1.0, 1.005, 0.995, 1.01, 0.99, 1.015.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Pitch center | 60 – 3000 Hz | Base pitch for all 6 voices. |
| **Y (k2)** | Brightness / decay | Dull – Bright | Low = muted string, fast decay. High = bright ring, long sustain. |

**Sound character:** Rain on a kalimba, or a prepared piano played by dice. Sparse, organic, constantly evolving.

---

### D-2: TubeResonance

**Noise excitation through 4 harmonic comb filters simulating a resonant tube.**

Four combs at 1x, 2x, 3x, 4x fundamental. Higher harmonics decay faster (feedback scaled by 1/harmonic).

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Tube length | 40 – 1000 Hz | Fundamental pitch. Low = long tube. High = short pipe. |
| **Y (k2)** | Excitation | Self-resonance – Breath | Low = singing bowl drone. High = breathy wind-through-pipe. |

**Sound character:** Hollow drone to breathy pipe texture. Harmonic series is clearly audible — sounds like a real resonant object.

---

### D-3: FormantNoise

**White noise through 3 bandpass filters at vowel formant frequencies.**

Interpolates between 5 vowel shapes (A, E, I, O, U) using 3 SVF bandpass filters.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Vowel morph | A → E → I → O → U | Smooth interpolation between vowel formants. |
| **Y (k2)** | Resonance | Q 1.0 – 5.0 | Wide/breathy to sharp/nasal vowels. |

**Formant table (Hz):**

| Vowel | F1 | F2 | F3 |
|-------|-----|------|------|
| A | 730 | 1090 | 2440 |
| E | 660 | 1720 | 2410 |
| I | 270 | 2290 | 3010 |
| O | 570 | 840 | 2410 |
| U | 300 | 870 | 2240 |

**Sound character:** Robotic choir whispering vowels. Sweep X for ghostly "aaah → eeeh → iiih → oooh → uuuh."

---

### D-4: BowedMetal

**Noise exciting 8 high-Q resonators at dense metallic frequency ratios.**

Eight 2-pole resonators (Q ~300–500) at plate vibration mode ratios. Denser than NoiseBells = more reverberant.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Fundamental pitch | 30 – 2030 Hz | All 8 resonators scale proportionally. |
| **Y (k2)** | Struck → Bowed | Short ring – Infinite sustain | Low = cymbal hit. High = bowed singing bowl. |

**Resonator ratios:** 1.000, 1.183, 1.506, 1.741, 2.098, 2.534, 2.917, 3.483

**Sound character:** Dense metallic reverberance. Bowed cymbal or gong. More diffuse than NoiseBells.

---

### D-5: FlangeNoise

**Pink noise through a flanger effect.**

Swept comb filter on noise. Classic "jet engine whoosh."

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Flange rate | 0.05 – 5 Hz | Slow sweep to fast warble. |
| **Y (k2)** | Flange depth | Subtle – Deep | Phase-like to dramatic comb sweep. |

**Sound character:** Jet plane flyover, ocean through metal tube, wind in rotating vent.

---

### D-6: NoiseBells

**White noise exciting 4 sharp 2-pole resonators at bell-like inharmonic ratios.**

Extremely high Q (~625) creates laser-sharp peaks. Clear ringing tones from noise.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Bell pitch | 80 – 4080 Hz | Fundamental. All 4 resonators scale. |
| **Y (k2)** | Inharmonicity | Chime → Gong | Near-harmonic (wind chime) to stretched (gamelan). |

**Resonator ratios (vary with Y):**

| Resonator | Y=0 (chime) | Y=1 (gong) |
|-----------|-------------|------------|
| 1 | 1.00x | 1.00x |
| 2 | 1.59x | 2.48x |
| 3 | 2.33x | 4.33x |
| 4 | 3.14x | 6.35x |

**Sound character:** Clear bell tones from noise. Visible on spectrum analyzer as 4 needle-sharp peaks.

---

### D-7: NoiseHarmonics

**White noise through a wavefolder, then resonant bandpass filter.**

Wavefolder creates harmonics from noise (DC bias controlled). SVF filter (Q=3) selects a frequency band.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Wavefold intensity | Gentle – Heavy | More folding = denser harmonics. |
| **Y (k2)** | Filter frequency | 100 – 8100 Hz | Selects which band is heard. |

**Sound character:** Aggressive buzzing. Like an angry bee swarm at a specific pitch. Grittier than plain filtered noise.

---

### D-8: IceRain

**Sparse sample-and-hold events with long reverb tails.**

Random stepped values at controllable rate into Freeverb (high damping).

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Droplet density | 0.5 – 200 Hz | Sparse plinks to dense rain. |
| **Y (k2)** | Reverb size | Small – Vast | Dry/close to cathedral-like. |

**Sound character:** Crystalline drops in a cave. Sparse plinks with shimmering tails. Very spatial and ambient.

---

### D-9: DroneBody

**Two cross-modulated FM sines through a wavefolder.**

Slightly detuned sines with block-delayed cross-feedback FM. Wavefolder adds harmonics.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Drone pitch | 20 – 520 Hz | Fundamental of both sines. |
| **Y (k2)** | Detune + Fold | Clean – Rich | Y=0: clean hum. Y=1: beating + growling overtones. |

**Sound character:** Deep meditative drone. Tibetan singing bowl meets power transformer. Good for sustained bass.

---

## Bank E: Chaos Machines

Deterministic chaos, algorithmic processes, and digital manipulation. Mathematical systems that hover between order and noise.

---

### E-0: LogisticNoise

**Logistic map chaotic oscillator: `x(n+1) = r * x(n) * (1 - x(n))`**

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Chaos parameter r | 3.5 – 4.0 | 3.5=periodic. 3.57=chaos onset. 3.83=period-3 window. 4.0=full chaos. |
| **Y (k2)** | Output rate | Slow – Fast | Held samples (buzzy tone) to per-sample (noise-like at r=4). |

**Sound character:** Mathematical structure dissolving into chaos. Sweep r to hear windows of order breaking into noise. The period-3 window at r~3.83 is particularly striking.

---

### E-1: HenonDust

**Henon attractor: `x' = 1 - a*x^2 + y, y' = b*x`**

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Parameter a | 1.15 – 1.40 | Low=periodic. ~1.2=period-doubling. 1.4=full chaos. |
| **Y (k2)** | Parameter b | 0.15 – 0.35 | Attractor shape and crackle density. |

**Sound character:** Alien vinyl surface noise. Almost-repeating patterns that never quite repeat. Living, deterministic crackle.

---

### E-2: CellularSynth

**1D cellular automaton (32 cells) controlling 8 sawtooth oscillators at harmonics of 55 Hz.**

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | CA rule | 0 – 255 | Rule 30=chaotic. Rule 90=fractal. Rule 110=complex. |
| **Y (k2)** | Evolution speed | Glacial – Rapid | How fast the CA evolves. |

**Sound character:** Alien music box that rewrites its own score. Some rules = steady drone, others = constant mutation.

---

### E-3: StochasticPulse

**Poisson-distributed impulses through a resonant bandpass filter (Q=4.5).**

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Impulse density | 5 – 505 /sec | Sparse pings to dense crackling. |
| **Y (k2)** | Filter frequency | 80 – 8080 Hz | Pitch of the resonant ring. |

**Sound character:** Geiger counter meets resonant drum. Random metallic pings with clear musical pitch.

---

### E-4: BitShift

**Two sawtooth oscillators combined via XOR digital logic.**

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Oscillator 1 freq | 20 – 5020 Hz | First sawtooth. |
| **Y (k2)** | Oscillator 2 ratio | 1:1 – 1:8 | Simple ratios = structured. Irrational = maximum complexity. |

**Sound character:** Broken NES / ZX Spectrum. Hard digital edges, rapidly shifting sum/difference tones. Aggressive 8-bit.

---

### E-5: FeedbackFM

**Single sine with sample-delayed self-feedback into its own phase.**

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Carrier frequency | 20 – 5020 Hz | Pitch. |
| **Y (k2)** | Feedback | 0 – 2.5 rad | 0=sine. ~0.8=bright. ~1.5=metallic. ~2.0=gritty. 2.5=noise. |

**Sound character:** The full timbral spectrum in one knob. Classic DX7 operator technique. Sweet spot at Y~1.0 = vocal/nasal quality.

---

### E-6: RunawayFilter

**White noise through a self-oscillating SVF (Q ~4.5–4.99).**

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Filter frequency | 30 – 10030 Hz | Pitched self-oscillation tone. |
| **Y (k2)** | Noise injection | Whistle – Noise | Y=0: clean sine whistle. Y=1: resonant noise. Transition zone = sweet spot. |

**Sound character:** Pitched whistle fighting noise. Like tuning a shortwave radio. Fragile tone that breaks and reforms.

---

### E-7: GlitchLoop

**Noise captured into a loop with progressive bit-depth degradation.**

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Loop length | 10 – 200 ms | Short = buzz. Long = stutter. |
| **Y (k2)** | Degradation rate | Stable – Rapid | How fast the loop crumbles. Refills when bit depth hits 1. |

**Sound character:** Digital decay. Loop captures, degrades, collapses to 1-bit, refreshes. Rhythmic destruction cycle.

---

### E-8: SubHarmonic

**Square wave with frequency dividers at /2, /3, /5, /7.**

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Fundamental frequency | 100 – 4100 Hz | Source square wave pitch. |
| **Y (k2)** | Subharmonic mix | /2 only → All four | Progressive: Y=0: octave below. Y=0.33: +fifth below. Y=0.66: +major 3rd down. Y=1: +minor 7th down. |

**Subharmonic intervals at 440 Hz:** /2=220 Hz, /3=~147 Hz, /5=88 Hz, /7=~63 Hz

**Sound character:** Deep rumbling sub-bass. Earth-shaking low end at full Y with low fundamental. Tectonic.

---

### E-9: DualAttractor

**Two coupled Lorenz attractors driving two sine oscillators.**

Lorenz system: `dx/dt = sigma(y-x)`, `dy/dt = x(rho-z)-y`, `dz/dt = xy-beta*z`. sigma=10, rho=28, beta=8/3.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Coupling strength | 0 – 5 | 0=independent chaos. 5=tones lock/unlock in bursts. |
| **Y (k2)** | Base frequency | 30 – 2030 Hz | Center pitch the attractors orbit. |

**Sound character:** Two drunk musicians trying to play in unison. Moments of harmony interrupted by chaos. Organic, alive, never repeating.

---

## Bank F: Stochastic

Random processes generating pulses, transients, and noise textures. The pulse-based algorithms (F-0 through F-4) exploit the Noise Plethora's internal DC blocker to transform random stepped values into unique transient/pulse shapes — a character exclusive to this module's architecture.

---

### F-0: PulseWander

**Smooth random walk generating organic pulse patterns.**

A random target value is chosen periodically, and a one-pole LP filter smoothly tracks it. The module's DC blocker transforms held values into distinctive pulse shapes with natural attack/decay envelopes.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Rate | 5 – 400 Hz | How often a new random target is chosen. Low = sparse, isolated pulses. High = dense, overlapping transients. |
| **Y (k2)** | Smoothness | Instant – Smooth | k2=0: instant jumps (sharp S&H pulses). k2=1: smooth glide between targets (softer, rounder transients). The LP filter alpha auto-scales with rate. |

**Sound character:** Organic, irregular pulse train. Each pulse has a unique amplitude from the random walk. The smoothness control morphs from sharp clicks to rounded bumps. Unlike a regular clock, the randomness creates a "breathing" quality.

---

### F-1: TwinPulse

**Two random walks with controllable correlation creating crossed pulse patterns.**

Two independent random walk generators running at the same rate. The correlation control blends the second walk toward the first, creating patterns that range from fully independent (complex interference) to identical (simple doubled pulse).

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Rate | 5 – 400 Hz | Speed of both random walks. |
| **Y (k2)** | Correlation | Independent – Identical | 0 = two completely unrelated pulse streams (complex polyrhythmic interference). 1 = both outputs identical (single pulse stream, louder). Sweet spot around 0.3–0.7 where patterns partially align. |

**Sound character:** Two overlapping pulse patterns. At low correlation, a complex, polyrhythmic texture where pulses sometimes align and sometimes don't. At high correlation, they merge into a single stream. The interplay creates a rhythmic quality that pure randomness lacks.

---

### F-2: QuantPulse

**Random values quantized to N bits, creating stepped pulse patterns.**

Generates random values quantized to a variable number of discrete levels (2^bits). At 1 bit: binary (2 levels). At 6 bits: 64 levels. The DC blocker creates pulses whose amplitude is quantized to these levels.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Rate | 5 – 500 Hz | Clock speed. How often a new quantized random value is generated. |
| **Y (k2)** | Bit depth | 1 – 6 bits (continuous) | Quantization resolution. 1 bit = binary pulse (on/off). 2 bits = 4 levels. 6 bits = 64 levels (nearly continuous). Continuous morphing between bit depths. |

**Sound character:** Quantized random pulses. At low bits: harsh, digital, with clearly discrete amplitude steps. At high bits: smoother, approaching the character of PulseWander. The bit quantization adds a lo-fi, digital texture to the random process.

---

### F-3: ShapedPulse

**Random with variable probability distribution shape.**

The random values follow different probability distributions depending on Y. Uniform (flat — all values equally likely), triangular (values near center more likely), or peaked/gaussian (values strongly concentrated near center, rare extremes).

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Rate | 5 – 500 Hz | Clock speed. |
| **Y (k2)** | Distribution | Uniform → Triangular → Gaussian | 0 = uniform (all amplitudes equally likely — evenly distributed pulses). 0.5 = triangular (sum of 2 uniforms — moderate peak clustering). 1.0 = peaked (sum of 4 uniforms, central limit theorem — most pulses near zero, rare large ones). |

**Sound character:** The distribution shape is audible. Uniform: even, balanced pulse amplitudes. Peaked: mostly quiet with occasional loud spikes — creates a "crackling" or "dripping" quality where large events are rare surprises.

---

### F-4: ShiftPulse

**4-stage shift register creating evolving pulse sequences.**

A shift register where random values enter stage 1 and shift down to stages 2, 3, 4 on each clock tick. All 4 stages are mixed in the output with adjustable weighting, creating patterns that evolve as values propagate through the chain.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Clock rate | 5 – 300 Hz | Speed of the shift register clock. |
| **Y (k2)** | Stage mix | Stage 1 only → All 4 | Progressive activation: Y=0: only stage 1 (fastest changing). Y=0.33: adds stage 2 (delayed echo). Y=0.66: adds stage 3. Y=1.0: all 4 stages equally mixed. The delayed repetitions create rhythmic pattern echoes. |

**Sound character:** A pulse pattern that develops delayed echoes of itself. At low Y: simple random pulses. As Y increases, you hear the same values repeated 1, 2, 3 steps later, creating a cascading effect — like an echo chamber for random events.

---

### F-5: MetallicNoise

**6 square wave oscillators at inharmonic frequencies mixed and highpass filtered.**

The classic technique used in the TR-808 cymbal and hi-hat circuits: six pulse oscillators at non-harmonic frequency ratios, summed together. The resulting waveform has a dense, metallic spectral character that no single oscillator can produce. A highpass filter controls brightness.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Pitch multiplier | 0.5x – 2.5x | Scales all 6 base frequencies together. At 1.0x: original frequencies. Below 1.0: lower, darker. Above: higher, brighter. |
| **Y (k2)** | Brightness | 500 – 15500 Hz HPF | Highpass filter cutoff. Low = full-bodied metallic noise. High = thin, sizzling hi-hat territory. |

**Base frequencies (Hz):** 205.3, 304.4, 369.6, 522.7, 800.6, 1053.4

**Sound character:** Unmistakably metallic. At low HPF: thick, crash cymbal-like. At high HPF: thin, closed hi-hat sizzle. The 6 inharmonic frequencies create dense beating patterns that give the "shimmer" characteristic of real cymbals.

---

### F-6: NoiseSlew

**White noise with adjustable one-pole lowpass slew.**

A simple but effective texture generator: white noise passed through a one-pole LP filter with adjustable cutoff. At high cutoff: full noise. At low cutoff: smooth, slowly undulating random wave. A mix control adds raw noise texture on top of the slewed signal.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Smoothness | Smooth – Noisy | LP filter coefficient. Low = very smooth, slowly varying random wave. High = barely filtered, noise-like. |
| **Y (k2)** | Texture mix | Clean – Textured | Blends raw noise on top of the slewed signal. 0 = only slewed output. 1 = slewed + 30% raw noise for gritty texture. |

**Sound character:** The full spectrum from smooth random undulation to white noise in one algorithm. At X=0: a gentle, wandering random wave. At X=1: nearly unfiltered noise. The texture mix adds a "grain" to the smooth version without overpowering it.

---

### F-7: NoiseBurst

**Sporadic bursts of white noise with silence between them.**

Generates discrete events: random-length bursts of noise separated by silence. Each burst has a natural fade-out envelope (20-sample ramp) to prevent clicks. The density controls how often bursts occur.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Burst density | ~1/sec – ~130/sec | Average burst rate. Low = sparse, isolated events. High = dense but never continuous — always gaps between bursts. |
| **Y (k2)** | Burst length | 1 ms – 100 ms | Duration of each noise burst. Short = percussive clicks. Long = noise "grains." |

**Sound character:** Sporadic noise events in silence. Like a Geiger counter, or rain hitting a window irregularly. At high density with short bursts: crackling granular texture. At low density with long bursts: isolated noise "drops" with natural decay.

---

### F-8: LFSRNoise

**16-bit Linear Feedback Shift Register with selectable tap configurations.**

A classic LFSR pseudo-random sequence generator with 8 different feedback tap polynomials. Different taps produce different sequence lengths and spectral characteristics. Output quantized to 16 discrete levels (top 4 bits) for a recognizable stepped character.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | Clock rate | 5 – 500 Hz | How fast the register shifts. Low = slow stepped pattern. High = fast digital noise with pitch. |
| **Y (k2)** | Tap selection | 8 configurations | Selects from 8 feedback polynomials. Each produces a different sequence pattern — some nearly random, some with audible repetition. Sweep to find sweet spots. |

**Sound character:** Digital pseudo-random sequences with a lo-fi, quantized character. Different taps create different "flavors" of digital noise — some more tonal, some more chaotic. The 16-level quantization gives it a distinctly stepped, 4-bit retro quality.

---

### F-9: DualPulse

**Two sample-and-hold generators running at independent rates, mixed together.**

Two independent S&H circuits, each generating random values at their own clock rate. The outputs are mixed 50/50, creating complex polyrhythmic stepped patterns from the interaction of two independent clocks.

| Parameter | Control | Range | Effect |
|-----------|---------|-------|--------|
| **X (k1)** | S&H 1 rate | 5 – 500 Hz | Clock speed of the first generator. |
| **Y (k2)** | S&H 2 rate | 5 – 500 Hz | Clock speed of the second generator. Independent of X. |

**Sound character:** Two overlapping random step patterns. When the two rates are similar: slow beating between the patterns. When rates are very different: a fast pattern modulated by a slow one. At simple ratios (2:1, 3:2): hints of regularity emerge. At complex ratios: maximally unpredictable interaction.
