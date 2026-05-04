# FRAKTAL 303 — Aufbauanleitung

## Was ist das hier?

Ein AU/VST3 Plugin-Synthesizer mit drei ungewöhnlichen Wellenformen:
- **Mandelbrot** — fraktale Iteration als Klang
- **Lorenz** — chaotischer Attractor, niemals periodisch
- **FM Feedback** — selbst-modulierender Operator

---

## Schritt 1 — Dateien in Projucer-Projekt einbinden

1. Projucer öffnen
2. Neues Projekt: **Audio Plug-In**
   - Name: `FraktalSynth303`
   - Plugin Formats: ✓ AU, ✓ VST3
3. Im Projucer links: **"Source Files"** anklicken
4. Alle `.h` und `.cpp` Dateien aus dem `Source/` Ordner hier hinzufügen:
   - Rechtsklick auf "Source Files" → "Add existing files"
   - Alle 6 Dateien auswählen:
     - `PluginProcessor.h` / `.cpp`
     - `PluginEditor.h` / `.cpp`
     - `WaveEngine.h`
     - `LadderFilter.h`
     - `SynthSound.h`
     - `SynthVoice.h`

---

## Schritt 2 — C++17 aktivieren

Im Projucer:
- Links: **"Exporters"** → **"Xcode (macOS)"**
- Unter "Extra compiler flags": `-std=c++17`

---

## Schritt 3 — Bauen

Im Projucer: **"Save and Open in IDE"**

In Xcode: **Cmd + B**

Build Succeeded → Plugin liegt in:
```
~/Library/Audio/Plug-Ins/VST3/FraktalSynth303.vst3
~/Library/Audio/Plug-Ins/Components/FraktalSynth303.component
```

---

## Schritt 4 — In DAW laden

- DAW neu starten (Logic / Ableton / Reaper)
- Plugin-Scanner: `FraktalSynth303` suchen
- Auf MIDI-Track laden
- MIDI-Noten spielen → Klang

---

## Parameter

| Parameter | Beschreibung |
|-----------|-------------|
| WAVE | Mandelbrot / Lorenz / FM Feedback |
| CUTOFF | Filterfrequenz (20–18k Hz) |
| RESO | Resonanz — vorsichtig über 0.8 |
| DRIVE | Soft-Clip Verzerrung |
| ENV> | Wie stark die Filterhüllkurve aufmacht |
| ATK/DEC/SUS/REL | Amplituden-Hüllkurve |
| LFO RATE | LFO-Geschwindigkeit (moduliert Filter) |
| LFO DEPTH | LFO-Tiefe |
| VOL | Master-Lautstärke |

---

## Klangtipps

**303-ähnlicher Klang:**
- Wave: Mandelbrot
- Cutoff: ~600 Hz
- Reso: 0.7
- ENV>: 0.8
- Decay: 0.2

**Lorenz-Textur (ambient):**
- Wave: Lorenz
- Cutoff: 800 Hz
- LFO Depth: 0.4, Rate: 0.15
- Attack: 0.3, Release: 1.5

**FM-Chaos:**
- Wave: FM Feedback
- Drive: 0.6
- Reso: 0.5
- ENV>: 1.0, Decay: 0.05

---

## Probleme?

**"Header not found"** → Dateien wurden nicht korrekt im Projucer hinzugefügt (Schritt 1)

**Kein Klang** → MIDI-Track? Plugin auf Instrument-Spur, nicht Audio-Spur

**Xcode Fehler "std::tanh not found"** → C++17 nicht aktiviert (Schritt 2)
