#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>

// ═══════════════════════════════════════════════════════════════════
//  GrayScottOscillator  –  Reaktions-Diffusions-Synthese
//
//  Zwei "chemische Stoffe" U und V reagieren in einer
//  eindimensionalen Röhre (64 Zellen, zyklische Randbedingungen).
//  Die V-Konzentration wird als dynamisches Wavetable gelesen.
//
//  Klangeigenschaften je nach Feed/Kill:
//    feed=0.025, kill=0.060  → stabile Pulse (Solitonen)
//    feed=0.035, kill=0.065  → wandernde Wellen, metallisch
//    feed=0.055, kill=0.062  → labyrinthartige Strukturen (Standard)
//    feed=0.012, kill=0.050  → mitotische Zellteilung, organisch
//
//  Das System erzeugt Klänge die weder periodisch noch reines
//  Rauschen sind — emergente, biologisch klingende Texturen.
//  Kein anderer Software-Synthesizer nutzt dieses Prinzip.
// ═══════════════════════════════════════════════════════════════════
class GrayScottOscillator
{
public:
    static constexpr int CELLS = 64;

    GrayScottOscillator() { init(); }

    void setSampleRate (double sr) { sampleRate = sr; }

    // Parameter (werden per Block vom SynthVoice gesetzt)
    float feed   = 0.055f;
    float kill   = 0.062f;
    float Da     = 1.0f;
    float Db     = 0.5f;
    float volume = 0.7f;

    void noteOn (int midi, float vel)
    {
        midiNote = midi;
        velocity = vel;
        phase    = 0.0;
        subCount = 0;
        init();
        active   = true;
    }

    void noteOff() {}
    void setActive (bool a) { active = a; }
    bool isActive() const   { return active; }

    float getNextSample()
    {
        if (!active) return 0.f;

        // Sub-Sampling: GS-System wird nur alle SUB_RATE Samples integriert
        // → spart CPU, klingt weich durch Unterabtastung
        if (++subCount >= SUB_RATE)
        {
            subCount = 0;
            stepGS();
        }

        // Wavetable-Readout: Phase läuft mit MIDI-Frequenz vorwärts
        const double freq = juce::MidiMessage::getMidiNoteInHertz (midiNote);
        phase += freq / sampleRate;
        if (phase >= 1.0) phase -= 1.0;

        // Lineare Interpolation in V-Array
        const double pos = phase * CELLS;
        const int    ia  = static_cast<int>(pos) % CELLS;
        const int    ib  = (ia + 1) % CELLS;
        const float  fr  = static_cast<float>(pos - std::floor(pos));
        const float  val = V[ia] * (1.f - fr) + V[ib] * fr;

        // V liegt typisch 0–0.5 → normalisieren zu [-1,1]
        // running mean für DC-Kompensation
        dcEstimate += (val - dcEstimate) * 0.0001f;
        const float centered = val - dcEstimate;
        const float out = juce::jlimit (-1.f, 1.f, centered * 4.f);

        return out * volume * velocity;
    }

private:
    static constexpr int   SUB_RATE     = 8;
    static constexpr int   WARMUP_STEPS = 400;  // Vor Audio-Start warm laufen lassen
    static constexpr float DT           = 0.2f; // Zeitschritt (stabil für Da≤1, Db≤0.5)

    std::array<float, CELLS> U{}, V{};
    std::array<float, CELLS> Ubuf{}, Vbuf{};
    double phase     = 0.0;
    double sampleRate = 44100.0;
    float  dcEstimate = 0.1f;
    int    subCount  = 0;
    int    midiNote  = 60;
    float  velocity  = 1.f;
    bool   active    = false;

    void init()
    {
        // Homogenes Feld U=1, V=0
        U.fill (1.f);
        V.fill (0.f);

        // Kleines Seed in der Mitte
        auto& rng = juce::Random::getSystemRandom();
        for (int c = CELLS / 2 - 4; c <= CELLS / 2 + 4; ++c)
        {
            U[c] = 0.50f + (rng.nextFloat() - 0.5f) * 0.1f;
            V[c] = 0.25f + rng.nextFloat() * 0.05f;
        }
        Ubuf = U;
        Vbuf = V;
        dcEstimate = 0.1f;

        // Aufwärm-Iterationen: Muster entfalten sich
        for (int i = 0; i < WARMUP_STEPS; ++i)
            stepGS();
    }

    void stepGS()
    {
        for (int c = 0; c < CELLS; ++c)
        {
            const int   lft = (c - 1 + CELLS) % CELLS;
            const int   rgt = (c + 1) % CELLS;
            const float u   = U[c], v = V[c];
            const float laU = U[lft] - 2.f * u + U[rgt];
            const float laV = V[lft] - 2.f * v + V[rgt];
            const float uvv = u * v * v;

            Ubuf[c] = juce::jlimit (0.f, 1.f, u + DT * (Da * laU - uvv + feed * (1.f - u)));
            Vbuf[c] = juce::jlimit (0.f, 1.f, v + DT * (Db * laV + uvv - (feed + kill) * v));
        }
        U.swap (Ubuf);
        V.swap (Vbuf);
    }
};
