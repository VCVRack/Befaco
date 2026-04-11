# Noise Plethora — Stereo Mode Guide

*Befaco Noise Plethora v1.5 + Banks D, E & F Extension*

## Table of Contents

- [What It Does](#what-it-does)
- [How to Activate (VCV Rack)](#how-to-activate-vcv-rack)
- [How to Activate (Hardware)](#how-to-activate-hardware)
- [Controls in Stereo Mode](#controls-in-stereo-mode)
- [How It Works Internally](#how-it-works-internally)
- [Stereo Patch Ideas](#stereo-patch-ideas)

---

## What It Does

Stereo mode transforms the Noise Plethora from two independent mono generators into a single **stereo sound source**. Generator B automatically mirrors Generator A's algorithm, and the B knobs (XB/YB) are repurposed as stereo controls.

The result: a wide stereo field from a single algorithm, where the left and right channels share the same pitch but differ in timbral character. The difference varies slowly over time, creating organic stereo movement.

---

## How to Activate (VCV Rack)

1. **Right-click** on the Noise Plethora module
2. Under "Stereo," check **"Stereo Mode"**
3. Both display dots light up to confirm stereo is active
4. To deactivate: right-click and uncheck

---

## How to Activate (Hardware)

**Double-click** the Program/Bank encoder (two quick clicks within 300ms):

| Gesture | Action |
|---------|--------|
| Single click | Toggle between Gen A and Gen B (normal) |
| **Double-click** | **Toggle stereo mode** |
| Hold 400ms | Enter/exit bank mode |

When stereo mode is active:
- Both dots on the 7-segment display light up simultaneously
- The display briefly shows "St" on activation and "no" on deactivation
- Both digits show the same program number

---

## Controls in Stereo Mode

| Control | Normal Mode | Stereo Mode |
|---------|------------|-------------|
| **XA knob** | k1 for Gen A | k1 for **both** (pitch/primary param) |
| **YA knob** | k2 for Gen A | k2 for **both** (timbre/secondary param) |
| **XB knob** | k1 for Gen B | **Stereo Width** (0=mono, 1=wide) |
| **YB knob** | k2 for Gen B | **Stereo Movement** (LFO speed for width modulation) |
| **CV XA** | CV for k1 A | CV for k1 **both** |
| **CV YA** | CV for k2 A | CV for k2 **both** |
| **CV XB** | CV for k1 B | Not used (manual only) |
| **CV YB** | CV for k2 B | Not used (manual only) |
| **Encoder** | Select program for A or B | Select program for **both** |
| **Filter A** | Filter for Gen A | Filter for **left channel** |
| **Filter B** | Filter for Gen B | Filter for **right channel** |

**Note:** The analog filters remain fully independent in stereo mode. This is a feature — setting the two filters differently (e.g., LP on left, HP on right) adds an additional layer of stereo character on top of the digital decorrelation.

---

## How It Works Internally

```
                    XA (k1) ─────────────────────→ Gen A ──→ Filter A ──→ LEFT
                    YA (k2) ─────────────────────→   │
                                                      │
                                                      │ (same algorithm)
                                                      │
                    XA (k1) ─────────────────────→ Gen B ──→ Filter B ──→ RIGHT
                    YA (k2) + stereo offset ─────→   │
                                                      │
                    XB ──→ offset amount              │
                    YB ──→ offset LFO speed ──────────┘
```

**The stereo offset is applied to k2 (timbre), not k1 (pitch).** This is critical — it means both channels always play at the same frequency, but with slightly different timbral character. This creates a natural, phase-free stereo width without the pitch-detuning artifacts of traditional chorus effects.

The offset follows a slow sine LFO:

```
offset = XB × 0.4 × sin(2π × YB × 0.3 × time)
```

- **XB = 0**: offset is zero → mono (both channels identical)
- **XB = 0.5**: offset swings ±0.2 → moderate stereo width
- **XB = 1.0**: offset swings ±0.4 → maximum stereo width
- **YB = 0**: offset is static → fixed stereo spread
- **YB = 0.5**: offset moves slowly → gentle stereo movement
- **YB = 1.0**: offset moves faster → animated stereo field

---

## Stereo Patch Ideas

### 1. Immersive Bell Drone
- Program: **D-6 NoiseBells**
- Stereo Mode: ON
- XA: ~0.3 (medium bell pitch)
- YA: ~0.2 (nearly harmonic, chime-like)
- XB: 0.3 (moderate width)
- YB: 0.15 (very slow movement)
- Filter A: LP, cutoff medium, res low
- Filter B: LP, cutoff slightly higher than A
- *Result: wide, shimmering bell drone where each ear hears slightly different harmonic content*

### 2. Vocal Wash
- Program: **D-3 FormantNoise**
- Stereo Mode: ON
- XA: modulated by slow LFO (vowel morphing)
- YA: ~0.6 (moderate resonance)
- XB: 0.5 (wide)
- YB: 0.2 (slow drift)
- Filter A: BP, cutoff ~1kHz
- Filter B: BP, cutoff ~1.5kHz
- *Result: ghostly stereo choir, each ear whispers a slightly different vowel*

### 3. Metallic Stereo Texture
- Program: **D-4 BowedMetal**
- Stereo Mode: ON
- XA: ~0.4
- YA: ~0.8 (long sustain, bowed character)
- XB: 0.6 (wide separation)
- YB: 0.3 (moderate movement)
- Filter A: HP, cutoff low (full spectrum)
- Filter B: LP, cutoff medium (darker)
- *Result: left ear = bright metallic shimmer, right ear = dark resonant glow. Complementary stereo.*

### 4. Chaos Stereo Field
- Program: **E-9 DualAttractor**
- Stereo Mode: ON
- XA: ~0.5 (moderate coupling)
- YA: ~0.4
- XB: 0.4
- YB: 0.4
- *Result: two Lorenz attractors already create wandering tones. In stereo mode, the additional timbral offset makes each channel's attractor feel independent while remaining harmonically related.*

### 5. Wide Cluster
- Program: **B-0 ClusterSaw**
- Stereo Mode: ON
- XA: ~0.3 (low fundamental)
- YA: ~0.1 (tight cluster = thick unison)
- XB: 0.2 (subtle width)
- YB: 0.1 (very slow)
- *Result: massive wide chorused sawtooth wall. The subtle timbral difference between channels creates a huge sound from a simple cluster.*
