#pragma once
#include <JuceHeader.h>

// ═══════════════════════════════════════════════════════════════════
//  Arpeggiator
//  - Modi: Up / Down / UpDown / Random
//  - Rate: 1/4 … 1/32, sync zur DAW-BPM
//  - Octave Range: 1–4
//  - Hält gehaltene Noten und arpeggiiert sie
// ═══════════════════════════════════════════════════════════════════

class Arpeggiator
{
public:
    enum class Mode { Up = 0, Down = 1, UpDown = 2, Random = 3 };

    void reset()
    {
        heldNotes.clear();
        stepIndex   = 0;
        goingUp     = true;
        sampleCount = 0;
        lastNote    = -1;
    }

    void setSampleRate (double sr) { sampleRate = sr; }

    void setBPM (float b)   { bpm = b; }
    void setMode (int m)    { mode = static_cast<Mode>(juce::jlimit (0, 3, m)); }
    void setDivision (int d){ division = d; }   // 0=1/4 1=1/8 2=1/16 3=1/32
    void setOctaves (int o) { octaveRange = juce::jlimit (1, 4, o); }
    void setEnabled (bool e){ enabled = e; }
    bool isEnabled() const  { return enabled; }

    // Ruf diese Funktion auf bevor du den Synthesiser renderst.
    // Sie schreibt neue Note-On/Off-Messages in den outBuffer.
    void process (const juce::MidiBuffer& inMidi,
                  juce::MidiBuffer&       outMidi,
                  int                     numSamples)
    {
        // Noten aus inMidi sammeln
        for (const auto meta : inMidi)
        {
            auto msg = meta.getMessage();
            if (msg.isNoteOn())
            {
                if (!heldNotes.contains (msg.getNoteNumber()))
                    heldNotes.add (msg.getNoteNumber());
                heldNotes.sort();
            }
            else if (msg.isNoteOff())
            {
                heldNotes.removeFirstMatchingValue (msg.getNoteNumber());
                if (heldNotes.isEmpty() && lastNote >= 0)
                {
                    outMidi.addEvent (juce::MidiMessage::noteOff (1, lastNote), 0);
                    lastNote = -1;
                }
            }
            else
            {
                outMidi.addEvent (msg, meta.samplePosition);
            }
        }

        if (!enabled || heldNotes.isEmpty()) return;

        // Schrittlänge in Samples berechnen
        double divFactor = 1.0;
        switch (division)
        {
            case 0: divFactor = 1.0;   break;  // 1/4
            case 1: divFactor = 0.5;   break;  // 1/8
            case 2: divFactor = 0.25;  break;  // 1/16
            case 3: divFactor = 0.125; break;  // 1/32
        }
        int stepSamples = static_cast<int>((60.0 / bpm) * divFactor * sampleRate);
        if (stepSamples < 1) stepSamples = 1;

        for (int s = 0; s < numSamples; ++s)
        {
            if (sampleCount <= 0)
            {
                sampleCount = stepSamples;

                // Alten Ton stoppen
                if (lastNote >= 0)
                    outMidi.addEvent (juce::MidiMessage::noteOff (1, lastNote), s);

                // Nächsten Schritt wählen
                int totalSteps = heldNotes.size() * octaveRange;
                stepIndex = juce::jlimit (0, totalSteps - 1, stepIndex);

                int noteIdx   = stepIndex % heldNotes.size();
                int octOffset = (stepIndex / heldNotes.size()) * 12;
                int note      = juce::jlimit (0, 127,
                                    heldNotes[noteIdx] + octOffset);

                outMidi.addEvent (juce::MidiMessage::noteOn (1, note, (uint8_t)100), s);
                lastNote = note;

                // Index vorrücken
                advanceStep (totalSteps);
            }
            --sampleCount;
        }
    }

private:
    juce::Array<int> heldNotes;
    Mode   mode        = Mode::Up;
    int    division    = 1;         // 1/8 default
    int    octaveRange = 1;
    bool   enabled     = false;
    bool   goingUp     = true;
    int    stepIndex   = 0;
    int    lastNote    = -1;
    int    sampleCount = 0;
    double sampleRate  = 44100.0;
    float  bpm         = 120.0f;

    void advanceStep (int totalSteps)
    {
        switch (mode)
        {
            case Mode::Up:
                stepIndex = (stepIndex + 1) % totalSteps;
                break;
            case Mode::Down:
                stepIndex = (stepIndex - 1 + totalSteps) % totalSteps;
                break;
            case Mode::UpDown:
                if (goingUp) {
                    ++stepIndex;
                    if (stepIndex >= totalSteps - 1) { stepIndex = totalSteps - 1; goingUp = false; }
                } else {
                    --stepIndex;
                    if (stepIndex <= 0) { stepIndex = 0; goingUp = true; }
                }
                break;
            case Mode::Random:
                stepIndex = juce::Random::getSystemRandom().nextInt (totalSteps);
                break;
        }
    }
};
