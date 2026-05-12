# FRAKTAL 611

**A synthesizer that doesn't sound like anything else.**

AU / VST3 plugin synthesizer built around chaotic, fractal and emergent synthesis methods. Where other synthesizers generate periodic waveforms, Fraktal 611 generates signals that never exactly repeat — using mathematics borrowed from chaos theory, reaction-diffusion systems and fractal geometry.

Built for experimental techno, industrial, ambient, drone, noise and sound design.

![Platform: macOS](https://img.shields.io/badge/Platform-macOS-lightgrey)
![Format: VST3 / AU](https://img.shields.io/badge/Format-VST3%20%2F%20AU-orange)
![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)

---

## Installation

### Quick install (recommended)

Copy the plugin files to the following locations:

| File | Destination |
|---|---|
| `Fraktal611.app` | `/Applications/` |
| `Fraktal611.vst3` | `/Library/Audio/Plug-Ins/VST3/` |
| `Fraktal611.component` | `/Library/Audio/Plug-Ins/Components/` |

After copying, restart your DAW or rescan plugins. In Logic Pro: Logic Pro menu → Plug-In Manager → Rescan.

### Presets

Presets are saved as `.xml` files inside category subfolders:

```
~/Documents/Fraktal611/Presets/
    Bass/       Lead/       Pad/        Drone/
    Pluck/      Dark/       Organic/    Broken/
    Chaos/      Experiment/ ARP/        Stutter/
```

Copy any existing presets into the matching subfolder — they will appear in the category dropdown automatically on next launch.

### Samples

Samples are not installed to a fixed location. The file path is stored inside each preset. When loading a preset that uses a sample, click **LOAD SAMPLE** to relink it, then **SAVE** to store the new path.

> **When sharing presets:** include both the `.xml` preset files and the sample files. The recipient will need to relink samples via LOAD SAMPLE, since absolute paths differ between machines.

---

## Features

### Oscillators

**OSC 1 & OSC 2** — two independent oscillators with 8 waveform modes:

| Mode | Description |
|---|---|
| Lorenz | Static wavetable built from the Lorenz attractor |
| FM Feedback | Metallic, aggressive FM with quadratic feedback |
| Granular | Soft, organic grain-based texture |
| Saw | Band-limited sawtooth (32 harmonics) |
| Square | Band-limited square with PWM |
| Drift | Slightly inharmonic partials that breathe and shift |
| **Lorenz Live** | Real-time Lorenz attractor — evolves every sample, never repeats |
| **Weierstrass** | Fractal additive synthesis — continuous everywhere, differentiable nowhere |

Independent pitch (±2 octaves), fine detune (±50 cents), PWM and volume per oscillator. Per-oscillator LP/HP filter.

**Morph** — blend continuously across all waveforms on a single axis.

### Sample Engines

- **OSC 3 — Sample Player:** load WAV/AIFF/FLAC/MP3, pitch-tracked to MIDI, loop modes, adjustable start/end points, ±24 octave pitch shift
- **OSC 4 — Grain Stutter:** grain envelope (Hann window), grain rate up to 500 Hz, acceleration, reverse mode, ±3x speed range

### Granular Layer

- **GRAN 1** — slow atmospheric grain clouds
- **GRAN 2** — fast glitchy grain texture

### Global Filter

Moog-style 4-pole ladder filter (Huovilainen model). Types: LP4 · HP4 · BP2 · Notch. Cutoff, Resonance, Drive, Filter Envelope Amount.

### Modulation

- **LFO 1** — filter cutoff modulation, Mod Wheel (CC1) controls depth
- **LFO 2** — independent LFO with ADSR envelope, targets: Pitch / Cutoff / PWM / Morph / LFO1 Rate
- **Poly-Temporal LFO** — multiply both LFOs by irrational numbers (φ, π, √2, √3, e) so they are mathematically never in sync
- Amp ADSR, Filter ADSR, Portamento, Pitch Bend

### FX

| Effect | Description |
|---|---|
| Chaos | Lorenz-based chaotic pitch drift |
| Fractal | Bifurcation waveshaper — from soft saturation to full deterministic chaos |
| Phase Dest | Non-linear phase modulation with feedback |
| Time Fold | Temporal waveshaping with mirror textures |
| Stutter | Rhythmic gating, free or DAW-synced (1/4 through 1/128, including triplets, quintuplets, septuplets) |
| Delay | Stereo delay, up to 3 seconds |
| Reverb | Schroeder network reverb |

### Advanced Synthesis Modules (Row 3)

These are unique to Fraktal 611 — no other synthesizer implements these.

**Lorenz Navigator** — the Lorenz attractor runs in real-time at audio rate. ρ (Rho) is the critical parameter: below 24.74 the system is periodic, above it the system enters deterministic chaos. Playing this transition live is unlike anything in conventional synthesis.

**Gray-Scott RD** — a one-dimensional reaction-diffusion system (the same mathematics behind leopard spots and zebra stripes) generates emergent audio patterns. Feed and Kill rates determine whether the system produces stable pulses, traveling waves or labyrinthine structures. No other synthesizer uses this principle.

**Weierstrass** — fractal additive synthesis based on the Weierstrass function: a signal that is continuous everywhere but differentiable nowhere. Dimension parameter moves from tonal (1.0) through metallic-rough (1.5) toward near-noise with internal fractal structure (→2.0).

**Evolutionary Drift** — parameters drift slowly by themselves, like a living system. Three modes: Brownian (random walk), Orbital (drifts around starting values), Avalanche (mostly still, occasional large jumps). Master volume and oscillator volumes are never touched.

### Other

- **Arpeggiator** — Up / Down / UpDown / Random, 1/4–1/32, 1–4 octave range, DAW BPM sync
- **Unison** — stereo spread with detuning, 6 polyphonic voices
- **Preset system** — 12 categories, saved as XML, RANDOM button for instant patch generation
- **MIDI Learn** — right-click any knob to assign a CC, saved with presets
- **PANIC** — stops all notes and clears delay/reverb buffers
- **Oscilloscope** — live waveform display, colour changes based on active effects

---

## Building from Source

**Requirements:** [JUCE](https://juce.com) 7 or 8 · Xcode (macOS) · C++17

```bash
git clone https://github.com/richblaha/FraktalSynth303.git
# Open FraktalSynth303.jucer in Projucer
# Export to Xcode and build
```

In Projucer: enable AU and VST3 formats, set C++ standard to C++17 under Xcode exporter → Extra Compiler Flags: `-std=c++17`

---

## Sound Design Starting Points

**303-style acid**
Wave: Lorenz · Cutoff: 600 Hz · Reso: 0.7 · Filter Env: 0.8 · Decay: 0.2

**Chaos ambient texture**
Wave: Lorenz Live · ρ near 24.74 · LFO Depth: 0.4, Rate: 0.15 · Attack: 0.3, Release: 1.5

**Industrial FM noise**
Wave: FM Feedback · Drive: 0.6 · Reso: 0.5 · Filter Env: 1.0, Decay: 0.05 · Fractal: 0.4

**Living drone**
Gray-Scott ON · Feed: 0.035, Kill: 0.065 · Evolutionary Drift: Orbital · Release: 3s

**Fractal percussion**
Wave: Weierstrass · Dim: 1.7 · Stutter synced · Fractal: 0.6 · Attack: 0ms

---

## System Requirements

- macOS 11 (Big Sur) or later
- Apple Silicon and Intel both supported (Universal Binary)
- Any AU or VST3 compatible DAW (Logic Pro, Ableton Live, Reaper, Bitwig, Cubase)

---

## License

Released under the **GNU General Public License v3.0**. Any derivative work must also be released under GPL v3. See [LICENSE](LICENSE) for full terms.

---

## Feedback & Issues

If something breaks or you have ideas, open an issue on GitHub.

*Fraktal 611 — Lorenz Live · Gray-Scott RD · Weierstrass · Evolutionary Drift · Poly-Temporal LFO*
