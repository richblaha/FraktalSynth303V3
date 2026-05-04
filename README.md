# FraktalSynth303

**A synthesizer that doesn't sound like anything else.**

Built for noise, experimental techno, industrial, ambient and glitch music. FraktalSynth303 is deliberately unstable — in the best possible way.

![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)
![Platform: macOS](https://img.shields.io/badge/Platform-macOS-lightgrey)
![Format: VST3 / AU](https://img.shields.io/badge/Format-VST3%20%2F%20AU-orange)

---

## Installation

### macOS
1. Download the latest release from the [Releases](../../releases) page
2. Run the installer `.pkg`
3. VST3 installs to: `/Library/Audio/Plug-Ins/VST3/`
4. AU installs to: `/Library/Audio/Plug-Ins/Components/`
5. Restart your DAW

### Manual install
Copy `FraktalSynth303.vst3` and/or `FraktalSynth303.component` to the folders above.

---

## Features

### Oscillators
- **OSC 1 & OSC 2** — two independent oscillators with 6 waveform modes each:
  - *Lorenz* — chaotic attractor, rich in overtones, never quite repeating
  - *FM Feedback* — metallic, aggressive FM with quadratic feedback
  - *Granular* — soft, organic grain-based texture
  - *Saw* — band-limited sawtooth (32 harmonics)
  - *Square* — band-limited square with PWM
  - *Drift* — soft, slightly inharmonic partials that breathe and shift
- Independent pitch (±2 octaves), fine detune (±50 cents), PWM, volume per OSC
- **Morph** — blend continuously across all 6 waveforms on a single axis
- **Morph Auto** — LFO-driven automatic morphing

### Per-OSC Filter
Each of the 4 sound sources has its own independent 1-pole filter:
- LP or HP mode
- Cutoff control per oscillator
- Runs before the global filter

### Sample Player (OSC 3)
- Load WAV, AIFF, FLAC or MP3
- Pitch-tracked to MIDI note (root note selectable)
- Loop modes: Off / Forward / PingPong
- Adjustable start and end points
- Runs through same filter and FX chain as oscillators

### Grain Stutter (OSC 4)
- Second independent sample player with grain envelope (Hann window)
- **GRAIN** — gate rate 1–64 Hz: from slow pulse to hard stutter to tremolo
- **ACCEL** — playback speed: normal, slow, fast-forward
- Designed for rhythmic build-ups and freeze effects

### Granular Layer
- **GRAN 1** — slow, soft, atmospheric grain clouds
- **GRAN 2** — fast, glitchy grain texture
- Mix independently into the signal

### Global Filter
- **Moog-style 4-pole ladder filter** (Huovilainen model)
- Types: LP4 · HP4 · BP2 · Notch
- Cutoff, Resonance (self-oscillates), Drive (tanh saturation), Filter Envelope Amount
- LFO 1 modulation of cutoff

### Modulation
- **LFO 1** — filter cutoff modulation, depth via Mod Wheel (CC1)
- **LFO 2** — independent LFO with ADSR envelope, targets: Pitch / Cutoff / PWM / Morph
- **Amp ADSR** — full envelope
- **Filter ADSR** — separate envelope for cutoff
- **Portamento** — glide between notes
- **Pitch Bend** ±2 semitones

### FX
- **SMEAR** — Lorenz-based chaotic pitch drift
- **FRAKTAL** — iterative waveshaping: phasedistortion → wavefolder → recursive sine. Subtle at low values, full noise chaos at high values. Each oscillator sounds different through it.
- **PHASE DEST** — non-linear phase modulation with feedback
- **TIME FOLD** — temporal waveshaping with mirror textures
- **STUTTER** — rhythmic gating, free or DAW-synced including triplets (T), quintuplets (Q) and septuplets (S): 1/4 through 1/128

### Section Bypass
Toggle the entire FILTER, FX and LFO2 sections on/off for instant A/B comparison.

### Delay & Reverb
- Stereo delay — mix, time (up to 3s), feedback
- Reverb — Schroeder network, mix and size
- Both are global post-mix effects (not per-voice)

### Arpeggiator
- Modes: Up · Down · UpDown · Random
- Divisions: 1/4 · 1/8 · 1/16 · 1/32
- Octave range: 1–4
- DAW BPM sync

### Unison
- Stereo spread with detuning across voices
- 6 polyphonic voices

### Monophony
- MONO mode for bass lines and leads
- New note immediately cuts previous

### Preset System
- 12 categories: Bass · Lead · Pad · Drone · Pluck · Dark · Organic · Broken · Chaos · Experiment · ARP · Stutter
- Saved as XML files in `~/Documents/FraktalSynth303/Presets/`
- RANDOM button for instant patch generation

### MIDI Learn
- Right-click any knob to assign a MIDI CC
- CC mappings saved with presets

### PANIC Button
- Immediately stops all notes and clears delay/reverb buffer

### Oscilloscope + Keyboard
- Live waveform display
- Playable MIDI keyboard (C2–C7), color changes based on active effects

---

## Who it's for

Sound designers and producers working in:
**Industrial · Experimental techno · Ambient · Noise · Glitch · Drone · Soundtrack**

---

## Building from source

Requirements:
- [JUCE](https://juce.com) (tested with JUCE 7)
- Xcode (macOS) or Visual Studio (Windows)
- C++17

```bash
git clone https://github.com/richblaha/FraktalSynth303.git
# Open FraktalSynth303.jucer in Projucer
# Export to your IDE and build
```

---

## License

Released under the **GNU General Public License v3.0**

You are free to use, modify and distribute this software under the terms of the GPL v3. Any derivative work must also be released under GPL v3.

See [LICENSE](LICENSE) for full terms.

---

## Feedback & Issues

If something breaks or you have ideas, open an issue on GitHub. The best features have come from users.
