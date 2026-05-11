#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "FraktalLookAndFeel.h"

// MIDI-Learn-faehiger Slider (Rotary)
class MidiLearnSlider : public juce::Slider
{
public:
    MidiLearnSlider(Fraktal611AudioProcessor& p, const juce::String& id) : proc(p), paramId(id)
    {
        setSliderStyle(juce::Slider::Rotary);
        setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
    }
    void mouseDown(const juce::MouseEvent& e) override
    {
        if(e.mods.isRightButtonDown()){proc.startMidiLearn(paramId);return;}
        juce::Slider::mouseDown(e);
    }
    Fraktal611AudioProcessor& proc;
    juce::String paramId;
};

class Fraktal611AudioProcessorEditor : public juce::AudioProcessorEditor,
                                       public juce::Timer
{
public:
    Fraktal611AudioProcessorEditor(Fraktal611AudioProcessor&);
    ~Fraktal611AudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    Fraktal611AudioProcessor& processor;
    FraktalLookAndFeel laf;

    using SA=juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA=juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA=juce::AudioProcessorValueTreeState::ButtonAttachment;

    // ── OSC 1 ─────────────────────────────────────────────────────
    juce::ComboBox waveSelector;
    MidiLearnSlider osc1VolKnob{processor,"osc1Vol"},osc1FilterCutKnob{processor,"osc1FilterCutoff"};
    juce::ComboBox osc1OctBox;
    juce::ToggleButton osc1OnBtn{"ON"},osc1FilterOnBtn{"F"},osc1FilterHPBtn{"HP"};
    MidiLearnSlider pwmKnob{processor,"pwm"};
    juce::Label osc1VolLabel,osc1OctLabel,pwmLabel,osc1FilterCutLabel;
    std::unique_ptr<SA> osc1VolAt,osc1FilterCutAt,pwmAt;
    std::unique_ptr<CA> waveAt,osc1OctAt;
    std::unique_ptr<BA> osc1OnAt,osc1FilterOnAt,osc1FilterHPAt;

    // ── OSC 2 ─────────────────────────────────────────────────────
    juce::ComboBox wave2Selector;
    MidiLearnSlider osc2VolKnob{processor,"osc2Vol"},osc2DetuneKnob{processor,"osc2Detune"};
    MidiLearnSlider osc2PwmKnob{processor,"osc2Pwm"},osc2FilterCutKnob{processor,"osc2FilterCutoff"};
    juce::ComboBox osc2OctBox;
    juce::ToggleButton osc2OnBtn{"ON"},osc2FilterOnBtn{"F"},osc2FilterHPBtn{"HP"};
    juce::Label osc2VolLabel,osc2DetLabel,osc2OctLabel,osc2PwmLabel,osc2FilterCutLabel;
    std::unique_ptr<SA> osc2VolAt,osc2DetuneAt,osc2PwmAt,osc2FilterCutAt;
    std::unique_ptr<CA> wave2At,osc2OctAt;
    std::unique_ptr<BA> osc2OnAt,osc2FilterOnAt,osc2FilterHPAt;

    // ── Filter + LFO ──────────────────────────────────────────────
    MidiLearnSlider cutoffKnob{processor,"cutoff"},resonanceKnob{processor,"resonance"};
    MidiLearnSlider driveKnob{processor,"drive"},envAmtKnob{processor,"filterEnvAmt"};
    MidiLearnSlider lfoRateKnob{processor,"lfoRate"},lfoDepthKnob{processor,"lfoDepth"};
    juce::ComboBox filterTypeBox,lfoRateMultBox;
    juce::ToggleButton filterOnBtn{"FLT"},fxOnBtn{"FX"};
    juce::Label cutoffLabel,resLabel,driveLabel,envAmtLabel,lfoRateLabel,lfoDepthLabel,lfoMultLabel;
    std::unique_ptr<SA> cutoffAt,resonanceAt,driveAt,envAmtAt,lfoRateAt,lfoDepthAt;
    std::unique_ptr<CA> filterTypeAt,lfoRateMultAt;
    std::unique_ptr<BA> filterOnAt,fxOnAt;

    // ── ADSR ──────────────────────────────────────────────────────
    MidiLearnSlider attackKnob{processor,"attack"},decayKnob{processor,"decay"};
    MidiLearnSlider sustainKnob{processor,"sustain"},releaseKnob{processor,"release"};
    juce::Label attackLabel,decayLabel,sustainLabel,releaseLabel;
    std::unique_ptr<SA> attackAt,decayAt,sustainAt,releaseAt;

    // ── FX (Chaos, Fractal, Stutter, Morph) ──────────────────────
    MidiLearnSlider chaosKnob{processor,"chaos"},chaos2Knob{processor,"chaos2"};
    MidiLearnSlider stutterKnob{processor,"stutter"},morphKnob{processor,"morph"};
    MidiLearnSlider morphAutoKnob{processor,"morphAuto"},phaseDestKnob{processor,"phaseDest"};
    MidiLearnSlider timeFoldKnob{processor,"timeFold"};
    juce::ComboBox stutterDivBox;
    juce::Label chaosLabel,chaos2Label,stutterLabel,morphLabel,morphAutoLabel,phaseDestLabel,timeFoldLabel;
    std::unique_ptr<SA> chaosAt,chaos2At,stutterAt,morphAt,morphAutoAt,phaseDestAt,timeFoldAt;
    std::unique_ptr<CA> stutterDivAt;

    // ── SMP 1 ────────────────────────────────────────────────────
    juce::TextButton loadSmpBtn{"LOAD"};
    MidiLearnSlider smpStartKnob{processor,"smpStart"},smpEndKnob{processor,"smpEnd"};
    MidiLearnSlider osc3VolKnob{processor,"osc3Vol"},smpFilterCutKnob{processor,"smp1FilterCutoff"};
    MidiLearnSlider smpPitchOctKnob{processor,"smpPitchOct"};
    juce::ComboBox smpRootBox,smpLoopBox;
    juce::ToggleButton osc3OnBtn{"ON"},smpFilterOnBtn{"F"};
    juce::Label smpLabel,smpStartLabel,smpEndLabel,osc3VolLabel,smpPitchOctLabel,smpFilterCutLabel;
    std::unique_ptr<SA> smpStartAt,smpEndAt,osc3VolAt,smpFilterCutAt,smpPitchOctAt;
    std::unique_ptr<CA> smpRootAt,smpLoopAt;
    std::unique_ptr<BA> osc3OnAt,smpFilterOnAt;

    // ── SMP 2 ────────────────────────────────────────────────────
    juce::TextButton loadSmp2Btn{"LOAD"};
    MidiLearnSlider smp2StartKnob{processor,"smp2Start"},smp2EndKnob{processor,"smp2End"};
    MidiLearnSlider osc4VolKnob{processor,"osc4Vol"},smp2GrainRateKnob{processor,"smp2GrainRate"};
    MidiLearnSlider smp2StutterAccKnob{processor,"smp2StutterAcc"},smp2FilterCutKnob{processor,"smp2FilterCutoff"};
    MidiLearnSlider smp2PitchOctKnob{processor,"smp2PitchOct"};
    juce::ComboBox smp2RootBox;
    juce::ToggleButton osc4OnBtn{"ON"},smp2FilterOnBtn{"F"},smp2RevBtn{"REV"};
    juce::Label smp2Label,smp2StartLabel,smp2EndLabel,osc4VolLabel;
    juce::Label smp2GrainRateLabel,smp2StutterAccLabel,smp2PitchOctLabel,smp2FilterCutLabel;
    std::unique_ptr<SA> smp2StartAt,smp2EndAt,osc4VolAt,smp2GrainRateAt,smp2StutterAccAt,smp2FilterCutAt,smp2PitchOctAt;
    std::unique_ptr<CA> smp2RootAt;
    std::unique_ptr<BA> osc4OnAt,smp2FilterOnAt,smp2RevAt;

    // ── LFO 2 ────────────────────────────────────────────────────
    MidiLearnSlider lfo2RateKnob{processor,"lfo2Rate"},lfo2DepthKnob{processor,"lfo2Depth"};
    MidiLearnSlider lfo2AttackKnob{processor,"lfo2Attack"},lfo2DecayKnob{processor,"lfo2Decay"};
    MidiLearnSlider lfo2SustainKnob{processor,"lfo2Sustain"},lfo2ReleaseKnob{processor,"lfo2Release"};
    juce::ComboBox lfo2TargetBox,lfo2RateMultBox;
    juce::ToggleButton lfo2OnBtn{"ON"};
    juce::Label lfo2RateLabel,lfo2DepthLabel,lfo2AtkLabel,lfo2DecLabel,lfo2SusLabel,lfo2RelLabel,lfo2MultLabel;
    std::unique_ptr<SA> lfo2RateAt,lfo2DepthAt,lfo2AttackAt,lfo2DecayAt,lfo2SustainAt,lfo2ReleaseAt;
    std::unique_ptr<CA> lfo2TargetAt,lfo2RateMultAt;
    std::unique_ptr<BA> lfo2OnAt;

    // ── Arp ──────────────────────────────────────────────────────
    juce::ToggleButton arpOnBtn{"ON"};
    juce::ComboBox arpModeBox,arpDivBox,arpOctBox;
    juce::Label arpModeLabel,arpDivLabel,arpOctLabel;
    std::unique_ptr<CA> arpModeAt,arpDivAt,arpOctAt;
    std::unique_ptr<BA> arpOnAt;

    // ── Master ───────────────────────────────────────────────────
    MidiLearnSlider volumeKnob{processor,"volume"},delayMixKnob{processor,"delayMix"};
    MidiLearnSlider delayTimeKnob{processor,"delayTime"},delayFBKnob{processor,"delayFeedback"};
    MidiLearnSlider reverbMixKnob{processor,"reverbMix"},reverbSizeKnob{processor,"reverbSize"};
    MidiLearnSlider portaKnob{processor,"portamento"},unisonDetuneKnob{processor,"unisonDetune"};
    MidiLearnSlider gran1Knob{processor,"gran1"},gran2Knob{processor,"gran2"};
    juce::ToggleButton monoBtn{"MONO"};
    juce::TextButton panicBtn{"PANIC"};
    juce::Label volumeLabel,delayMixLabel,delayTimeLabel,delayFBLabel;
    juce::Label reverbMixLabel,reverbSizeLabel,portaLabel,uniDetLabel,gran1Label,gran2Label;
    std::unique_ptr<SA> volumeAt,delayMixAt,delayTimeAt,delayFBAt,reverbMixAt,reverbSizeAt;
    std::unique_ptr<SA> portaAt,unisonDetuneAt,gran1At,gran2At;
    std::unique_ptr<BA> monoAt;

    // ── Preset ───────────────────────────────────────────────────
    juce::TextButton prevPresetBtn{"<"},nextPresetBtn{">"};
    juce::TextButton savePresetBtn{"SAVE"},loadPresetFileBtn{"OPEN"};
    juce::Label presetNameLabel;

    // ── MIDI keyboard ────────────────────────────────────────────
    juce::MidiKeyboardComponent keyboard;

    // ── Oscilloscope ─────────────────────────────────────────────
    std::array<float,Fraktal611AudioProcessor::SCOPE_SIZE> scopeData{};

    // ─────────────────────────────────────────────────────────────
    // REIHE 3: neue Features
    // ─────────────────────────────────────────────────────────────

    // Lorenz Navigator
    MidiLearnSlider lorenzSigmaKnob{processor,"lorenzSigma"},lorenzRhoKnob{processor,"lorenzRho"};
    MidiLearnSlider lorenzBetaKnob{processor,"lorenzBeta"},lorenzDtKnob{processor,"lorenzDt"};
    juce::Label lorenzSigmaLabel,lorenzRhoLabel,lorenzBetaLabel,lorenzDtLabel;
    std::unique_ptr<SA> lorenzSigmaAt,lorenzRhoAt,lorenzBetaAt,lorenzDtAt;

    // Gray-Scott
    juce::ToggleButton gsOnBtn{"GS ON"};
    MidiLearnSlider gsFeedKnob{processor,"gsFeed"},gsKillKnob{processor,"gsKill"};
    MidiLearnSlider gsDaKnob{processor,"gsDa"},gsDbKnob{processor,"gsDb"};
    MidiLearnSlider gsVolKnob{processor,"gsVol"};
    juce::Label gsFeedLabel,gsKillLabel,gsDaLabel,gsDbLabel,gsVolLabel;
    std::unique_ptr<SA> gsFeedAt,gsKillAt,gsDaAt,gsDbAt,gsVolAt;
    std::unique_ptr<BA> gsOnAt;

    // Weierstrass
    MidiLearnSlider weiDimKnob{processor,"weiDimension"};
    juce::ComboBox weiLacBox,weiDepthBox;
    juce::Label weiDimLabel,weiLacLabel,weiDepthLabel;
    std::unique_ptr<SA> weiDimAt;
    std::unique_ptr<CA> weiLacAt,weiDepthAt;

    // Evolutionary Drift
    juce::ToggleButton driftOnBtn{"DRIFT"};
    MidiLearnSlider driftSpeedKnob{processor,"driftSpeed"},driftRangeKnob{processor,"driftRange"};
    juce::ComboBox driftModeBox;
    juce::Label driftSpeedLabel,driftRangeLabel,driftModeLabel;
    std::unique_ptr<SA> driftSpeedAt,driftRangeAt;
    std::unique_ptr<CA> driftModeAt;
    std::unique_ptr<BA> driftOnAt;

    // Poly-temporale LFOs
    juce::Label lfoMultR3Label,lfo2MultR3Label;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Fraktal611AudioProcessorEditor)
};
