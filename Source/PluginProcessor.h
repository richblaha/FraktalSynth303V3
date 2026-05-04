#pragma once
#include <JuceHeader.h>
#include "WaveEngine.h"
#include "SynthVoice.h"
#include "SynthSound.h"
#include "Arpeggiator.h"
#include "SampleOscillator.h"
#include <array>
#include <unordered_map>

// ═══════════════════════════════════════════════════════════════════
//  PresetManager
// ═══════════════════════════════════════════════════════════════════
class PresetManager
{
public:
    static const juce::StringArray& getCategories()
    {
        static juce::StringArray cats {
            "Bass", "Lead", "Pad", "Drone", "Pluck",
            "Dark", "Organic", "Broken", "Chaos", "Experiment",
            "ARP", "Stutter"
        };
        return cats;
    }
    static juce::File getPresetDir (const juce::String& category)
    {
        return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("Fraktal611").getChildFile ("Presets")
               .getChildFile (category);
    }
    static juce::StringArray getPresetsInCategory (const juce::String& category)
    {
        juce::StringArray names;
        auto dir = getPresetDir (category);
        if (dir.exists())
            for (auto& f : dir.findChildFiles (juce::File::findFiles, false, "*.xml"))
                names.add (f.getFileNameWithoutExtension());
        names.sort (true);
        return names;
    }
    static bool savePreset (juce::AudioProcessorValueTreeState& apvts,
                            const juce::String& category, const juce::String& name,
                            const juce::String& samplePath = {})
    {
        auto dir = getPresetDir (category);
        dir.createDirectory();
        auto state = apvts.copyState();
        auto xml   = state.createXml();
        if (!xml) return false;
        xml->setAttribute ("presetCategory", category);
        xml->setAttribute ("presetName",     name);
        // Sample-Pfad mitspeichern (leer = kein Sample)
        if (samplePath.isNotEmpty())
            xml->setAttribute ("samplePath", samplePath);
        return xml->writeTo (dir.getChildFile (name + ".xml"));
    }

    // Gibt den gespeicherten Sample-Pfad zurück (leer = keiner)
    static juce::String loadPreset (juce::AudioProcessorValueTreeState& apvts,
                                    const juce::String& category, const juce::String& name)
    {
        auto file = getPresetDir (category).getChildFile (name + ".xml");
        if (!file.existsAsFile()) return {};
        auto xml = juce::XmlDocument::parse (file);
        if (!xml) return {};
        auto tree = juce::ValueTree::fromXml (*xml);
        if (!tree.isValid()) return {};
        apvts.replaceState (tree);
        // Sample-Pfad zurückgeben damit der Editor das Sample laden kann
        return xml->getStringAttribute ("samplePath", {});
    }
};

// ═══════════════════════════════════════════════════════════════════
//  PluginProcessor
// ═══════════════════════════════════════════════════════════════════
class Fraktal611AudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int SCOPE_SIZE = 1024;

    Fraktal611AudioProcessor();
    ~Fraktal611AudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Fraktal611"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // MIDI-Learn
    void startMidiLearn (const juce::String& paramId);
    bool isMidiLearnActive() const { return midiLearnActive; }
    const std::unordered_map<int,juce::String>& getCCMap() const { return ccMap; }
    void panic();  // Alle Noten stoppen

    // Scope
    void getScopeData (std::array<float, SCOPE_SIZE>& out);

    // Synth-Zugriff für Editor
    juce::Synthesiser&    getSynth()         { return synth; }
    SharedSampleBuffer&   getSharedSample()  { return sharedSample; }
    SharedSampleBuffer&   getSharedSample2();

    // Keyboard State für Editor
    juce::MidiKeyboardState keyboardState;

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    void applyMidiLearnCC (const juce::MidiBuffer&);

    juce::Synthesiser  synth;
    WaveEngine         waveEngine;
    Arpeggiator        arp;
    SharedSampleBuffer sharedSample;

    // Globaler Delay + Reverb (stereo, post-mix)
    static constexpr int DELAY_BUF_SIZE = 131072; // ~3s @ 44100, Potenz von 2
    float globalDelayLine[2][DELAY_BUF_SIZE] = {};
    float globalReverbBuf[2][4]              = {};
    int   globalDelayWrite                   = 0;

    // Scope
    std::array<float, SCOPE_SIZE> scopeBuffer {};
    int                           scopeWritePos = 0;
    juce::CriticalSection         scopeLock;
    juce::AudioBuffer<float>      oscBuffer;

    // MIDI-Learn
    bool              midiLearnActive  = false;
    juce::String      midiLearnParamId;
    std::unordered_map<int, juce::String> ccMap;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Fraktal611AudioProcessor)
};
