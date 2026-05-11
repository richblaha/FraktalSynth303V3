#pragma once
#include <JuceHeader.h>
#include <unordered_map>
#include <vector>

// ═══════════════════════════════════════════════════════════════════
//  EvolutionaryDrift
//  Der Synthesizer "lebt" — Parameter verändern sich langsam
//  von selbst, wie ein biologisches System.
//
//  Modi:
//    Brownian  – Zufälliger Walk, kehrt nie zurück (wie Brownsche Bewegung)
//    Orbital   – Driftet um den Startwert herum, zieht sich zurück
//    Avalanche – Meist ruhig, gelegentlich große Sprünge (Power-Law)
//
//  Wichtig: setValueNotifyingHost ist in JUCE thread-safe für
//  AudioParameter und darf aus dem Audio-Thread aufgerufen werden.
// ═══════════════════════════════════════════════════════════════════
class EvolutionaryDrift
{
public:
    enum class Mode { Brownian = 0, Orbital = 1, Avalanche = 2 };

    bool enabled = false;

    // Muss nach APVTS-Erstellung aufgerufen werden
    void prepare (juce::AudioProcessorValueTreeState& a)
    {
        apvts = &a;
        storeCurrentAsOrigins();
    }

    // Speichert aktuelle Parameterwerte als "Heimat" für Orbital-Modus
    void storeCurrentAsOrigins()
    {
        if (!apvts) return;
        for (const auto& id : driftableParams)
            if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (apvts->getParameter (id)))
                origins[id] = p->convertFrom0to1 (p->getValue());
    }

    // Aufruf aus processBlock
    void process (int numSamples, float speed, float range, int modeInt)
    {
        if (!enabled || !apvts) return;

        accum += numSamples;

        // Intervall: speed=0 → ~10 Sek; speed=1 → ~100ms
        const float  logSpeed  = std::pow (juce::jmax (0.001f, speed), 0.4f);
        const int    interval  = juce::jmax (128, (int)(44100.f * 0.1f * (1.f - logSpeed * 0.9f)));

        if (accum < interval) return;
        accum = 0;

        const auto mode = static_cast<Mode> (modeInt);

        for (const auto& id : driftableParams)
        {
            auto* p = dynamic_cast<juce::RangedAudioParameter*> (apvts->getParameter (id));
            if (!p) continue;

            const auto& r    = p->getNormalisableRange();
            const float span = r.end - r.start;
            const float cur  = p->convertFrom0to1 (p->getValue());
            float       delta = 0.f;

            switch (mode)
            {
                case Mode::Brownian:
                    delta = (rng.nextFloat() * 2.f - 1.f) * range * span * 0.06f;
                    break;

                case Mode::Orbital:
                {
                    const float org    = origins.count (id) ? origins[id] : cur;
                    const float wander = (rng.nextFloat() * 2.f - 1.f) * range * span * 0.05f;
                    const float pull   = (org - cur) * 0.05f;  // Rückstellkraft
                    delta = wander + pull;
                    break;
                }

                case Mode::Avalanche:
                    // Meist kleine Änderungen, 6% Chance auf großen Sprung
                    if (rng.nextFloat() < 0.06f)
                        delta = (rng.nextFloat() * 2.f - 1.f) * range * span * 0.45f;
                    else
                        delta = (rng.nextFloat() * 2.f - 1.f) * range * span * 0.008f;
                    break;
            }

            const float newVal = juce::jlimit (r.start, r.end, cur + delta);
            p->setValueNotifyingHost (p->convertTo0to1 (newVal));
        }
    }

private:
    juce::AudioProcessorValueTreeState*        apvts = nullptr;
    juce::Random                               rng;
    int                                        accum = 0;
    std::unordered_map<juce::String, float>    origins;

    // Parameterliste die driften darf (bewusst ausgewählt – klingt musikalisch)
    const std::vector<juce::String> driftableParams
    {
        "cutoff", "resonance", "lfoRate", "lfoDepth",
        "morph", "phaseDest", "chaos", "timeFold",
        "lfo2Rate", "lfo2Depth", "filterEnvAmt",
        "lorenzRho", "lorenzSigma", "lorenzBeta",
        "gsFeed", "gsKill"
    };
};
