#pragma once
#include <JuceHeader.h>

// Minimal sound descriptor
struct SynthSound : public juce::SynthesiserSound
{
    bool appliesToNote    (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};
