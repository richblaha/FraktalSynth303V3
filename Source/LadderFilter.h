#pragma once
#include <JuceHeader.h>
#include <cmath>

// ═══════════════════════════════════════════════════════════════════
//  LadderFilter  –  Moog-style 4-pole (Huovilainen)
//  Types: LP4, HP4, BP2, Notch
// ═══════════════════════════════════════════════════════════════════

class LadderFilter
{
public:
    enum class Type { LP4 = 0, HP4 = 1, BP2 = 2, Notch = 3 };

    LadderFilter() { reset(); }

    void setSampleRate (double sr) { sampleRate = sr; }
    void setType (Type t)          { filterType = t; }

    void setParameters (float cutoffHz, float res)
    {
        cutoff    = juce::jlimit (20.0f, 20000.0f, cutoffHz);
        resonance = juce::jlimit (0.0f,  1.0f,     res);
        updateCoefficients();
    }

    float process (float x)
    {
        float res = resonance * resonance;
        float input = x - res * 2.2f * s[3];
        input *= (1.0f + res * 0.8f);
        input = std::tanh (input);

        updateStage (0, input);
        updateStage (1, s[0]);
        updateStage (2, s[1]);
        updateStage (3, s[2]);

        switch (filterType)
        {
            case Type::HP4:
                return x - s[3] - s[0];          // Highpass Näherung
            case Type::BP2:
                return s[1] - s[3];               // Bandpass
            case Type::Notch:
                return x - s[1];                  // Notch
            default:                               // LP4
                return s[3];
        }
    }

    void reset()
    {
        for (auto& v : s) v = 0.0f;
        for (auto& v : z) v = 0.0f;
    }

private:
    double sampleRate = 44100.0;
    float  cutoff     = 1000.0f;
    float  resonance  = 0.0f;
    float  g          = 0.0f;
    float  s[4]       = {};
    float  z[4]       = {};
    Type   filterType = Type::LP4;

    void updateCoefficients()
    {
        float wd = juce::MathConstants<float>::twoPi * cutoff;
        float T  = 1.0f / static_cast<float>(sampleRate);
        float wa = (2.0f / T) * std::tan (wd * T * 0.5f);
        g  = wa * T * 0.5f;
        g /= (1.0f + g);
    }

    void updateStage (int i, float input)
    {
        float v = (input - z[i]) * g;
        float out = v + z[i];
        z[i] = out + v;
        s[i] = out;
    }
};
