#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <vector>

// ═══════════════════════════════════════════════════════════════════
//  WaveEngine v5
//  6 Wellenformen, klar differenziert:
//
//  0  Lorenz    – chaotisch, reich an Obertönen, leicht rau
//  1  FM        – metallisch-aggressive FM-Färbung, viel Feedback
//  2  Granular  – weich, organisch, leicht verschwommen (soft!)
//  3  Saw       – klassisch bandbegrenzt, hell
//  4  Square    – hohl, mit PWM
//  5  Drift     – sehr weich: morphende Sinusobertöne (neu)
// ═══════════════════════════════════════════════════════════════════

class WaveEngine
{
public:
    static constexpr int TABLE_SIZE = 4096;  // höher = mehr Qualität
    static constexpr int NUM_WAVES  = 6;

    enum class WaveType { Lorenz=0, FMFeedback=1, Granular=2, Saw=3, Square=4, Drift=5 };

    WaveEngine()
    {
        buildLorenz();
        buildFMFeedback();
        buildGranular();
        buildSaw();
        buildSquare();
        buildDrift();
    }

    // ── Einzelner Sample aus Wavetable ────────────────────────────
    float getSample (WaveType type, double phase) const
    {
        return readTable (tableFor(type), phase);
    }

    // ── MORPH: 0=Lorenz … 5=Drift ─────────────────────────────────
    float getMorphSample (double phase, float morphPos) const
    {
        morphPos = juce::jlimit (0.0f, (float)(NUM_WAVES - 1), morphPos);
        int   wA = static_cast<int>(morphPos);
        int   wB = juce::jmin (wA + 1, NUM_WAVES - 1);
        float t  = morphPos - static_cast<float>(wA);
        float sA = readTableByIndex (wA, phase);
        float sB = readTableByIndex (wB, phase);
        float tA = std::cos (t * juce::MathConstants<float>::halfPi);
        float tB = std::sin (t * juce::MathConstants<float>::halfPi);
        return sA * tA + sB * tB;
    }

    // ── PolyBLEP Sägezahn ─────────────────────────────────────────
    static float sawPolyBLEP (double phase, double phaseInc)
    {
        float saw = static_cast<float>(2.0 * phase - 1.0);
        saw -= polyBLEP (phase, phaseInc);
        return saw;
    }

    // ── Square mit PWM ────────────────────────────────────────────
    static float squarePWM (double phase, double phaseInc, float pw)
    {
        pw = juce::jlimit (0.05f, 0.95f, pw);
        float s1 = static_cast<float>(2.0 * phase - 1.0) - polyBLEP(phase, phaseInc);
        double p2 = phase + pw; if (p2 >= 1.0) p2 -= 1.0;
        float s2 = static_cast<float>(2.0 * p2 - 1.0) - polyBLEP(p2, phaseInc);
        return (s1 - s2 - (2.0f * pw - 1.0f)) * 0.5f;
    }

    // ── "Drift" Echtzeit-Sample (phasenbasiert, kein Table) ───────
    // Wird von SynthVoice direkt aufgerufen für maximale Weichheit
    static float driftSample (double phase, float driftPhase1, float driftPhase2)
    {
        float t = static_cast<float>(phase) * juce::MathConstants<float>::twoPi;
        // Drei Sinusse mit leicht verschobenen Phasen → weich und organisch
        float s = std::sin (t)
                + 0.5f  * std::sin (2.0f * t + driftPhase1 * 0.3f)
                + 0.25f * std::sin (3.0f * t + driftPhase2 * 0.7f)
                + 0.12f * std::sin (4.0f * t - driftPhase1 * 0.5f)
                + 0.06f * std::sin (5.0f * t + driftPhase2 * 0.2f);
        return s / 1.93f;  // normalisieren
    }

private:
    std::vector<float> lorenz;
    std::vector<float> fmFeedback;
    std::vector<float> granularTable;
    std::vector<float> sawTable;
    std::vector<float> squareTable;
    std::vector<float> driftTable;

    float readTable (const std::vector<float>& t, double phase) const
    {
        double pos = phase * TABLE_SIZE;
        int ia = static_cast<int>(pos) % TABLE_SIZE;
        int ib = (ia + 1) % TABLE_SIZE;
        float fr = static_cast<float>(pos - std::floor(pos));
        return t[ia] * (1.0f - fr) + t[ib] * fr;
    }

    float readTableByIndex (int idx, double phase) const
    {
        const std::vector<float>* tbl = &lorenz;
        switch (idx) {
            case 1: tbl = &fmFeedback;   break;
            case 2: tbl = &granularTable; break;
            case 3: tbl = &sawTable;      break;
            case 4: tbl = &squareTable;   break;
            case 5: tbl = &driftTable;    break;
            default: break;
        }
        return readTable (*tbl, phase);
    }

    const std::vector<float>& tableFor (WaveType t) const
    {
        switch (t) {
            case WaveType::FMFeedback:  return fmFeedback;
            case WaveType::Granular:    return granularTable;
            case WaveType::Saw:         return sawTable;
            case WaveType::Square:      return squareTable;
            case WaveType::Drift:       return driftTable;
            default:                    return lorenz;
        }
    }

    static float polyBLEP (double t, double dt)
    {
        if (dt <= 0.0) return 0.0f;
        if (t < dt)        { double x=t/dt;       return (float)( x+x - x*x - 1.0); }
        if (t > 1.0 - dt)  { double x=(t-1.0)/dt; return (float)( x*x + x+x + 1.0); }
        return 0.0f;
    }

    // ── Lorenz: chaotisch, rau, viele Obertöne ───────────────────
    // Mehr Iterationen → stabilerer Attraktor, klarer chaotischer Charakter
    void buildLorenz()
    {
        lorenz.resize (TABLE_SIZE);
        const float sigma=10.f, rho=28.f, beta=2.6667f;
        float x=0.1f, y=0.f, z=20.f, dt=0.002f;
        // Länger aufwärmen
        for (int i=0; i<16000; ++i) {
            float dx=sigma*(y-x), dy=x*(rho-z)-y, dz=x*y-beta*z;
            x+=dx*dt; y+=dy*dt; z+=dz*dt;
        }
        for (int i=0; i<TABLE_SIZE; ++i) {
            float dx=sigma*(y-x), dy=x*(rho-z)-y, dz=x*y-beta*z;
            x+=dx*dt; y+=dy*dt; z+=dz*dt;
            // x+y mischen → mehr chaotische Färbung
            lorenz[i] = x * 0.6f + y * 0.4f;
        }
        normalise (lorenz);
        // Leichtes DC-Offset entfernen
        removeDC (lorenz);
    }

    // ── FM Feedback: metallisch, aggressiv ────────────────────────
    // Höheres Feedback-Verhältnis → mehr Obertöne, metallischer Charakter
    void buildFMFeedback()
    {
        fmFeedback.resize (TABLE_SIZE);
        float prev = 0.0f;
        for (int i=0; i<TABLE_SIZE; ++i) {
            float t  = (float)i / TABLE_SIZE;
            // Ratio 3.5 + starkes Feedback → aggressiv metallisch
            float ph = t * juce::MathConstants<float>::twoPi * 3.5f
                     + t * 12.0f * prev * prev; // quadratisches Feedback = mehr Obertöne
            float s  = std::sin (ph);
            // Zweite FM-Schicht
            float ph2 = t * juce::MathConstants<float>::twoPi * 7.0f + s * 4.0f;
            fmFeedback[i] = s * 0.7f + std::sin(ph2) * 0.3f;
            prev = s;
        }
        normalise (fmFeedback);
        removeDC (fmFeedback);
    }

    // ── Granular: weich, verschwommen, organisch ──────────────────
    // Gemischt aus gefensterten Sinusoiden → kein harter Anstieg
    void buildGranular()
    {
        granularTable.resize (TABLE_SIZE);
        // Viele leise, leicht verstimmte Sinusse → weiche Textur
        for (int i=0; i<TABLE_SIZE; ++i) {
            float t = (float)i / TABLE_SIZE;
            float tp = t * juce::MathConstants<float>::twoPi;
            float s = 0.0f;
            // Harmonische mit Gaussian-Einfärbung → weich
            float env = std::exp (-std::pow ((t - 0.5f) * 2.5f, 2.0f));
            s += std::sin (tp * 1.0f) * 1.0f;
            s += std::sin (tp * 1.98f + 0.3f) * 0.6f;   // leicht verstimmt
            s += std::sin (tp * 2.97f + 0.7f) * 0.35f;
            s += std::sin (tp * 3.96f + 1.1f) * 0.18f;
            s += std::sin (tp * 4.95f + 0.5f) * 0.09f;
            s += std::sin (tp * 5.94f + 1.4f) * 0.04f;
            // Fensterung → sanftere Transienten
            float win = 0.5f - 0.5f * std::cos (tp);
            granularTable[i] = s * (0.6f + 0.4f * win);
        }
        normalise (granularTable);
        removeDC (granularTable);
    }

    // ── Saw: klassisch bandbegrenzt ───────────────────────────────
    void buildSaw()
    {
        sawTable.resize (TABLE_SIZE);
        for (int i=0; i<TABLE_SIZE; ++i) {
            float t = (float)i / TABLE_SIZE;
            float s = 0.0f;
            for (int h=1; h<=32; ++h)
                s += std::sin (h * t * juce::MathConstants<float>::twoPi)
                     / (float)h * (h==1 ? 1.0f : 1.0f);
            sawTable[i] = s * (2.0f / juce::MathConstants<float>::pi);
        }
        normalise (sawTable);
    }

    // ── Square: bandbegrenzt, ungerade Harmonische ────────────────
    void buildSquare()
    {
        squareTable.resize (TABLE_SIZE);
        for (int i=0; i<TABLE_SIZE; ++i) {
            float t = (float)i / TABLE_SIZE;
            float s = 0.0f;
            for (int h=1; h<=32; h+=2)
                s += std::sin (h * t * juce::MathConstants<float>::twoPi) / (float)h;
            squareTable[i] = s * (4.0f / juce::MathConstants<float>::pi);
        }
        normalise (squareTable);
    }

    // ── Drift: sehr weich, langsam morphende Obertöne ─────────────
    // Additive Sinusse mit leicht inharmonischen Verhältnissen
    void buildDrift()
    {
        driftTable.resize (TABLE_SIZE);
        for (int i=0; i<TABLE_SIZE; ++i) {
            float t  = (float)i / TABLE_SIZE;
            float tp = t * juce::MathConstants<float>::twoPi;
            float s  = 0.0f;
            // Leicht inharmonische Partialtöne → organisch, "atmet"
            s += std::sin (tp * 1.000f) * 1.000f;
            s += std::sin (tp * 1.995f) * 0.500f;   // fast 2., leicht tief
            s += std::sin (tp * 2.987f) * 0.280f;
            s += std::sin (tp * 3.978f) * 0.140f;
            s += std::sin (tp * 4.968f) * 0.070f;
            s += std::sin (tp * 5.957f) * 0.030f;
            // Tiefpass-Fensterung → kein hartes Aliasing
            float win = 0.54f - 0.46f * std::cos (tp); // Hann
            driftTable[i] = s * (0.7f + 0.3f * win);
        }
        normalise (driftTable);
        removeDC (driftTable);
    }

    static void normalise (std::vector<float>& t)
    {
        float peak = 0.0f;
        for (float s : t) peak = std::max (peak, std::abs(s));
        if (peak > 1e-6f) for (float& s : t) s /= peak;
    }

    static void removeDC (std::vector<float>& t)
    {
        float sum = 0.0f;
        for (float s : t) sum += s;
        float dc = sum / (float)t.size();
        for (float& s : t) s -= dc;
        normalise (t);
    }
};
