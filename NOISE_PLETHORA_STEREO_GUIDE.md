# Noise Plethora — Stereo Mode Guide

*Befaco Noise Plethora v1.5 + Banks D, E & F Extension*

## Table of Contents

- [What It Does](#what-it-does)
- [How to Activate (VCV Rack)](#how-to-activate-vcv-rack)
- [How to Activate (Hardware)](#how-to-activate-hardware)
- [Controls in Stereo Mode](#controls-in-stereo-mode)
- [How It Works Internally](#how-it-works-internally)
- [State Behavior](#state-behavior)
- [Stereo Patch Ideas](#stereo-patch-ideas)

---

## What It Does

Stereo mode transforms the Noise Plethora from two independent mono generators into a single **stereo sound source**. Generator B automatically mirrors Generator A's algorithm, and the B knobs (XB/YB) are repurposed as stereo panning controls.

The result: a wide stereo field from a single algorithm, where a sine LFO pans the sound between the left and right outputs by modulating channel amplitudes.

---

## How to Activate (VCV Rack)

1. **Right-click** on the Noise Plethora module
2. Under "Stereo," check **"Stereo Mode"**
3. Both display dots light up to confirm stereo is active
4. To deactivate: right-click and uncheck

---

## How to Activate (Hardware)

**Double-click** the Program/Bank encoder button (two clicks within 300 ms):

| Gesture | Action |
|---------|--------|
| Single click | Toggle between Gen A and Gen B (normal mode) |
| **Double-click** | **Toggle stereo mode** |
| Hold >400 ms | Enter/exit bank mode |

When stereo mode is active:
- Both decimal dots on the 7-segment display light up simultaneously
- Both digits show the same value (A's program number or bank letter)
- Single-click (A/B toggle) is disabled
- The module auto-exits bank mode upon entering stereo

---

## Controls in Stereo Mode

| Control | Normal Mode | Stereo Mode |
|---------|------------|-------------|
| **XA knob** | k1 for Gen A | k1 for **both** (pitch/primary param) |
| **YA knob** | k2 for Gen A | k2 for **both** (timbre/secondary param) |
| **XB knob** | k1 for Gen B | **Pan width** (0 = mono center, 1 = full L/R swing) |
| **YB knob** | k2 for Gen B | **Pan LFO speed** (0 = static, max = 1.5 Hz) |
| **CV XA** | CV for k1 A | CV for k1 **both** |
| **CV YA** | CV for k2 A | CV for k2 **both** |
| **CV XB** | CV for k1 B | Not used (manual only) |
| **CV YB** | CV for k2 B | Not used (manual only) |
| **Encoder turn** | Select program for A or B | Select program for **both** (A changes, B follows) |
| **Single click** | Toggle A/B | **Disabled** (locked to A) |
| **Hold >400 ms** | Bank mode | Bank mode (works normally) |
| **Double-click** | Enter stereo | **Exit stereo** (restores B) |
| **Filter A** | Filter for Gen A | Filter for **left channel** |
| **Filter B** | Filter for Gen B | Filter for **right channel** |

**Note:** The analog filters remain fully independent in stereo mode. Setting the two filters differently (e.g., LP on left, HP on right) adds an additional layer of stereo character on top of the digital panning.

---

## How It Works Internally

```
                XA (k1) ──→ Gen A ──→ ampA (gainL) ──→ Filter A ──→ LEFT
                YA (k2) ──→   │
                               │ (same algorithm, same params)
                XA (k1) ──→ Gen B ──→ ampB (gainR) ──→ Filter B ──→ RIGHT
                YA (k2) ──→   │
                               │
                XB ──→ pan width (amplitude)
                YB ──→ pan LFO speed ───────────────────┘
```

**The stereo effect uses real amplitude panning** — a sine LFO modulates the gain of each channel inversely:

```
pan    = width * sin(2pi * phase)
gainL  = (1 - pan) / 2
gainR  = (1 + pan) / 2
phase += speed^2 * 1.5 * dt     (time-based, independent of loop rate)
```

### Pan Width (XB)

| XB position | Pan range | Perceived effect |
|-------------|-----------|------------------|
| 0.0 | No panning | Mono (both channels equal, 0.5 / 0.5) |
| 0.5 | Half swing | Moderate stereo (0.25 – 0.75) |
| 1.0 | Full swing | Full pan (0.0 – 1.0, hard left to hard right) |

### Pan LFO Speed (YB)

The speed control has a **quadratic response** (`speed^2 * 1.5 Hz`) for fine control at low settings:

| YB position | LFO rate | Cycle time |
|-------------|----------|------------|
| 0.0 | 0 Hz | Static (no movement) |
| 0.1 | 0.015 Hz | ~67 seconds |
| 0.2 | 0.06 Hz | ~17 seconds |
| 0.3 | 0.135 Hz | ~7.4 seconds |
| 0.5 | 0.375 Hz | ~2.7 seconds |
| 0.7 | 0.735 Hz | ~1.4 seconds |
| 1.0 | 1.5 Hz | ~0.67 seconds |

### Display in Stereo Mode

The 7-segment display has two digits, each with a decimal dot. In stereo mode both dots light up simultaneously as the stereo indicator. Both digits mirror Generator A's current value:

| View | Display | Meaning |
|------|---------|---------|
| Program mode | `3.3.` | Program 3 on both generators, both dots = stereo active |
| Bank mode | `A.A.` | Bank A on both generators, both dots = stereo active |

In normal (non-stereo) mode only one dot is lit, indicating which generator (A or B) the encoder controls.

---

## State Behavior

### Entering Stereo (double-click)

1. Saves B's current bank, program, and A/B selection
2. Syncs B to A's algorithm
3. Forces encoder to control A (B follows automatically)
4. Auto-exits bank mode (so the stereo display indicator is visible)
5. Both decimal dots turn on

### During Stereo

- Encoder changes A's program/bank; B mirrors the change instantly
- Single click (A/B toggle) is disabled
- Hold >400 ms enters bank mode normally (both dots stay on)
- XB and YB control panning instead of Gen B parameters

### Exiting Stereo (double-click)

1. Restores channel gains to unity (1.0, 1.0)
2. Restores B's saved bank and program
3. Restores previous A/B selection
4. Both decimal dots return to normal behavior

---

## Stereo Patch Ideas

### 1. Immersive Bell Drone
- Program: **D-6 NoiseBells**
- XA: ~0.3 (medium bell pitch)
- YA: ~0.2 (nearly harmonic, chime-like)
- XB: 0.3 (moderate width)
- YB: 0.15 (very slow movement)
- Filter A: LP, cutoff medium, res low
- Filter B: LP, cutoff slightly higher than A
- *Result: wide, shimmering bell drone that slowly sweeps between speakers*

### 2. Vocal Wash
- Program: **D-3 FormantNoise**
- XA: modulated by slow LFO (vowel morphing)
- YA: ~0.6 (moderate resonance)
- XB: 0.5 (wide)
- YB: 0.2 (slow drift)
- Filter A: BP, cutoff ~1kHz
- Filter B: BP, cutoff ~1.5kHz
- *Result: ghostly stereo choir panning gently across the field*

### 3. Metallic Stereo Texture
- Program: **D-4 BowedMetal**
- XA: ~0.4
- YA: ~0.8 (long sustain, bowed character)
- XB: 0.6 (wide separation)
- YB: 0.3 (moderate movement)
- Filter A: HP, cutoff low (full spectrum)
- Filter B: LP, cutoff medium (darker)
- *Result: metallic shimmer sweeping left-right. Filter differences add tonal depth to the pan.*

### 4. Chaos Stereo Field
- Program: **E-9 DualAttractor**
- XA: ~0.5 (moderate coupling)
- YA: ~0.4
- XB: 0.4
- YB: 0.4
- *Result: two Lorenz attractors panning slowly. The chaotic timbral evolution combined with spatial movement creates an immersive, unpredictable field.*

### 5. Wide Cluster
- Program: **B-0 ClusterSaw**
- XA: ~0.3 (low fundamental)
- YA: ~0.1 (tight cluster = thick unison)
- XB: 0.2 (subtle width)
- YB: 0.1 (very slow)
- *Result: massive chorused sawtooth wall that breathes between speakers. Subtle width keeps it cohesive.*
