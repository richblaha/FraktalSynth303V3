#pragma once
#include <JuceHeader.h>
#include "WaveEngine.h"
#include "SampleOscillator.h"
#include "SynthVoice.h"
#include "Arpeggiator.h"
#include "PresetManager.h"
#include "EvolutionaryDrift.h"
#include <array>
#include <unordered_map>

class SynthSound : public juce::SynthesiserSound
{public:bool appliesToNote(int) override{return true;}bool appliesToChannel(int) override{return true;}};

class Fraktal611AudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int SCOPE_SIZE     = 512;
    static constexpr int DELAY_BUF_SIZE = 131072;

    Fraktal611AudioProcessor();
    ~Fraktal611AudioProcessor() override;
    void prepareToPlay(double,int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& l) const override
        {return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo()
             || l.getMainOutputChannelSet()==juce::AudioChannelSet::mono();}
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override{return true;}
    const juce::String getName() const override{return "Fraktal611";}
    bool acceptsMidi() const override{return true;}
    bool producesMidi() const override{return false;}
    double getTailLengthSeconds() const override{return 2.0;}
    int getNumPrograms() override{return 1;}
    int getCurrentProgram() override{return 0;}
    void setCurrentProgram(int) override{}
    const juce::String getProgramName(int) override{return "Init";}
    void changeProgramName(int,const juce::String&) override{}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    void getScopeData(std::array<float,SCOPE_SIZE>&);
    void startMidiLearn(const juce::String&);
    void applyMidiLearnCC(const juce::MidiBuffer&);
    void panic();
    bool isMidiLearnActive() const{return midiLearnActive;}
    const juce::String& getMidiLearnParamId() const{return midiLearnParamId;}
    juce::MidiKeyboardState keyboardState;
    juce::AudioProcessorValueTreeState apvts;
    SharedSampleBuffer sharedSample;
    static SharedSampleBuffer& getSharedSample2();
    Arpeggiator arp;
    PresetManager presetManager{apvts};

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    WaveEngine   waveEngine;
    juce::Synthesiser synth;
    juce::AudioBuffer<float> oscBuffer;
    EvolutionaryDrift drift;          // Evolutionaerer Drift
    std::array<float,SCOPE_SIZE> scopeBuffer{};
    int scopeWritePos=0;
    juce::CriticalSection scopeLock;
    float globalDelayLine[2][DELAY_BUF_SIZE]={};
    float globalReverbBuf[2][4]={};
    int   globalDelayWrite=0;
    bool  midiLearnActive=false;
    juce::String midiLearnParamId;
    std::unordered_map<int,juce::String> ccMap;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Fraktal611AudioProcessor)
};
