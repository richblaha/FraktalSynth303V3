#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OscilloscopeComponent : public juce::Component, public juce::Timer
{
public:
    OscilloscopeComponent(Fraktal611AudioProcessor& p):proc(p){startTimerHz(30);}
    void timerCallback() override { proc.getScopeData(scopeData); repaint(); }
    void paint(juce::Graphics& g) override
    {
        auto b=getLocalBounds().toFloat();
        g.fillAll(juce::Colour(0xFF080808));
        g.setColour(juce::Colour(0xFF1A1A1A)); g.drawRect(b,1.f);
        g.setColour(juce::Colour(0xFF161616));
        g.drawHorizontalLine((int)b.getCentreY(),b.getX(),b.getRight());
        juce::Path p; bool started=false;
        float cy=b.getCentreY(), w=b.getWidth();
        for(int i=0;i<Fraktal611AudioProcessor::SCOPE_SIZE;++i){
            float x=b.getX()+(float)i/Fraktal611AudioProcessor::SCOPE_SIZE*w;
            float y=cy-scopeData[i]*cy*0.82f;
            if(!started){p.startNewSubPath(x,y);started=true;}else p.lineTo(x,y);
        }
        g.setColour(juce::Colour(0xFFC8FF00));
        g.strokePath(p,juce::PathStrokeType(1.2f));
    }
private:
    Fraktal611AudioProcessor& proc;
    std::array<float,Fraktal611AudioProcessor::SCOPE_SIZE> scopeData{};
};

class MidiLearnSlider : public juce::Slider
{
public:
    MidiLearnSlider(Fraktal611AudioProcessor& p,const juce::String& id)
        :proc(p),paramId(id){}
    void mouseDown(const juce::MouseEvent& e) override
    {
        if(e.mods.isRightButtonDown()){
            juce::PopupMenu m;
            m.addItem(1,"MIDI Learn");
            for(auto& kv:proc.getCCMap())
                if(kv.second==paramId) m.addItem(2,"Clear CC "+juce::String(kv.first));
            m.showMenuAsync(juce::PopupMenu::Options(),[this](int r){
                if(r==1)proc.startMidiLearn(paramId);});
        } else juce::Slider::mouseDown(e);
    }
private:
    Fraktal611AudioProcessor& proc; juce::String paramId;
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
    const juce::Colour C_BG{0xFF080808},C_ACCENT{0xFFC8FF00},
                       C_DIM{0xFF2A2A2A},C_MUTED{0xFF555555};

    OscilloscopeComponent scope{processor};
    juce::MidiKeyboardComponent keyboard{processor.keyboardState,
        juce::MidiKeyboardComponent::horizontalKeyboard};

    // ── ComboBoxes ────────────────────────────────────────────────
    juce::ComboBox waveSelector,wave2Selector;
    juce::ComboBox osc1OctBox,osc2OctBox;
    juce::ComboBox filterTypeBox;
    juce::ComboBox stutterModeBox,stutterDivBox;
    juce::ComboBox lfo2TargetBox;
    juce::ComboBox arpModeBox,arpDivBox,arpOctBox;
    juce::ComboBox smpRootBox,smpLoopBox;
    juce::ComboBox smp2RootBox;

    // ── Toggles ───────────────────────────────────────────────────
    juce::ToggleButton osc1OnBtn{""},osc2OnBtn{""}; // Section-Header zeigt "OSC 1/2" bereits
    juce::ToggleButton osc3OnBtn{""}, osc4OnBtn{""};
    juce::ToggleButton osc1FOnBtn{"LP"},osc1FHPBtn{"HP"};
    juce::ToggleButton osc2FOnBtn{"LP"},osc2FHPBtn{"HP"};
    juce::ToggleButton smp1FOnBtn{"FILT"}, smp2FOnBtn{"FILT"};
    juce::ToggleButton secFilterBtn{"FILTER"},secFXBtn{"FX"},secLFO2Btn{"LFO2"};
    juce::ToggleButton monoBtn{"MONO"};
    juce::TextButton   panicBtn{"PANIC"};
    juce::ToggleButton arpOnBtn{"ARP ON"};

    // ── Knobs OSC1 ────────────────────────────────────────────────
    MidiLearnSlider pwmKnob     {processor,"pwm"},     osc1VolKnob  {processor,"osc1Vol"};
    MidiLearnSlider gran1Knob   {processor,"gran1"},   gran2Knob    {processor,"gran2"};
    MidiLearnSlider osc1FCKnob  {processor,"osc1FilterCutoff"};
    // OSC2
    MidiLearnSlider osc2VolKnob {processor,"osc2Vol"}, osc2DetKnob  {processor,"osc2Detune"};
    MidiLearnSlider osc2PwmKnob {processor,"osc2Pwm"};
    MidiLearnSlider osc2FCKnob  {processor,"osc2FilterCutoff"};
    // FILTER
    MidiLearnSlider cutoffKnob    {processor,"cutoff"},   resonanceKnob{processor,"resonance"};
    MidiLearnSlider filterEnvKnob {processor,"filterEnvAmt"},driveKnob{processor,"drive"};
    MidiLearnSlider lfoRateKnob   {processor,"lfoRate"},  lfoDepthKnob {processor,"lfoDepth"};
    // ADSR
    MidiLearnSlider attackKnob  {processor,"attack"},  decayKnob  {processor,"decay"};
    MidiLearnSlider sustainKnob {processor,"sustain"}, releaseKnob{processor,"release"};
    MidiLearnSlider portaKnob   {processor,"portamento"};
    // FX
    MidiLearnSlider chaosKnob    {processor,"chaos"},    chaos2Knob   {processor,"chaos2"};
    MidiLearnSlider stutterKnob  {processor,"stutter"},  morphKnob    {processor,"morph"};
    MidiLearnSlider morphAutoKnob{processor,"morphAuto"},phaseDestKnob{processor,"phaseDest"};
    MidiLearnSlider timeFoldKnob {processor,"timeFold"};
    // SMP1
    MidiLearnSlider osc3VolKnob  {processor,"osc3Vol"},  smpStartKnob {processor,"smpStart"};
    MidiLearnSlider smpEndKnob   {processor,"smpEnd"},   smp1FCKnob   {processor,"smp1FilterCutoff"};
    // SMP2
    MidiLearnSlider osc4VolKnob  {processor,"osc4Vol"},  smp2StartKnob{processor,"smp2Start"};
    MidiLearnSlider smp2EndKnob  {processor,"smp2End"};
    MidiLearnSlider smp2GrainKnob{processor,"smp2GrainRate"};
    MidiLearnSlider smp2AccKnob  {processor,"smp2StutterAcc"};
    MidiLearnSlider smp2FCKnob   {processor,"smp2FilterCutoff"};
    // LFO2
    MidiLearnSlider lfo2AttackKnob {processor,"lfo2Attack"}, lfo2DecayKnob  {processor,"lfo2Decay"};
    MidiLearnSlider lfo2RateKnob   {processor,"lfo2Rate"},   lfo2DepthKnob  {processor,"lfo2Depth"};
    MidiLearnSlider lfo2SustainKnob{processor,"lfo2Sustain"},lfo2ReleaseKnob{processor,"lfo2Release"};
    // MASTER
    MidiLearnSlider delayMixKnob {processor,"delayMix"},  delayTimeKnob{processor,"delayTime"};
    MidiLearnSlider delayFBKnob  {processor,"delayFeedback"};
    MidiLearnSlider reverbMixKnob{processor,"reverbMix"}, reverbSizeKnob{processor,"reverbSize"};
    MidiLearnSlider uniDetuneKnob{processor,"unisonDetune"},uniSpreadKnob{processor,"unisonSpread"};
    MidiLearnSlider volumeKnob   {processor,"volume"};

    // ── Labels ────────────────────────────────────────────────────
    juce::Label pwmLabel,osc1VolLabel,gran1Label,gran2Label,osc1FCLabel;
    juce::Label osc2VolLabel,osc2DetLabel,osc2PwmLabel,osc2FCLabel;
    juce::Label cutoffLabel,resonanceLabel,filterEnvLabel,driveLabel,lfoRateLabel,lfoDepthLabel;
    juce::Label attackLabel,decayLabel,sustainLabel,releaseLabel,portaLabel;
    juce::Label chaosLabel,chaos2Label,stutterLabel,morphLabel,morphAutoLabel,phaseDestLabel,timeFoldLabel;
    juce::Label osc3VolLabel,smpStartLabel,smpEndLabel,smp1FCLabel;
    juce::Label osc4VolLabel,smp2StartLabel,smp2EndLabel,smp2GrainLabel,smp2AccLabel,smp2FCLabel;
    juce::Label lfo2AttackLabel,lfo2DecayLabel,lfo2RateLabel,lfo2DepthLabel,lfo2SustainLabel,lfo2ReleaseLabel;
    juce::Label delayMixLabel,delayTimeLabel,delayFBLabel,reverbMixLabel,reverbSizeLabel;
    juce::Label uniDetuneLabel,uniSpreadLabel,volumeLabel;

    // ── Sample file labels + buttons ──────────────────────────────
    juce::Label      smpFileLabel, smp2FileLabel;
    juce::TextButton smpLoadBtn{"LOAD SAMPLE"}, smp2LoadBtn{"LOAD SAMPLE"};
    std::unique_ptr<juce::FileChooser> fileChooser, fileChooser2;

    // ── Preset ────────────────────────────────────────────────────
    juce::ComboBox   presetCategoryBox,presetNameBox;
    juce::TextButton presetSaveBtn{"SAVE"},presetLoadBtn{"LOAD"},randomBtn{"RANDOM"};
    juce::TextEditor presetNameEdit;

    // ── Attachments ───────────────────────────────────────────────
    using SA=juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA=juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA=juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<CA> waveAttach,wave2Attach,osc1OctAttach,osc2OctAttach;
    std::unique_ptr<CA> filterTypeAttach,stutterModeAttach,stutterDivAttach;
    std::unique_ptr<CA> lfo2TargetAttach,arpModeAttach,arpDivAttach,arpOctAttach;
    std::unique_ptr<CA> smpRootAttach,smpLoopAttach,smp2RootAttach;
    std::unique_ptr<BA> osc1OnAttach,osc2OnAttach,osc3OnAttach,osc4OnAttach;
    std::unique_ptr<BA> osc1FOnAttach,osc1FHPAttach,osc2FOnAttach,osc2FHPAttach;
    std::unique_ptr<BA> smp1FOnAttach,smp2FOnAttach;
    std::unique_ptr<BA> secFilterAttach,secFXAttach,secLFO2Attach,arpOnAttach,monoAttach;
    std::unique_ptr<SA> pwmAttach,osc1VolAttach,gran1Attach,gran2Attach,osc1FCAttach;
    std::unique_ptr<SA> osc2VolAttach,osc2DetAttach,osc2PwmAttach,osc2FCAttach;
    std::unique_ptr<SA> cutoffAttach,resonanceAttach,filterEnvAttach,driveAttach;
    std::unique_ptr<SA> lfoRateAttach,lfoDepthAttach;
    std::unique_ptr<SA> attackAttach,decayAttach,sustainAttach,releaseAttach,portaAttach;
    std::unique_ptr<SA> chaosAttach,chaos2Attach,stutterAttach,morphAttach;
    std::unique_ptr<SA> morphAutoAttach,phaseDestAttach,timeFoldAttach;
    std::unique_ptr<SA> osc3VolAttach,smpStartAttach,smpEndAttach,smp1FCAttach;
    std::unique_ptr<SA> osc4VolAttach,smp2StartAttach,smp2EndAttach;
    std::unique_ptr<SA> smp2GrainAttach,smp2AccAttach,smp2FCAttach;
    std::unique_ptr<SA> lfo2AttackAttach,lfo2DecayAttach,lfo2RateAttach,lfo2DepthAttach;
    std::unique_ptr<SA> lfo2SustainAttach,lfo2ReleaseAttach;
    std::unique_ptr<SA> delayMixAttach,delayTimeAttach,delayFBAttach;
    std::unique_ptr<SA> reverbMixAttach,reverbSizeAttach;
    std::unique_ptr<SA> uniDetuneAttach,uniSpreadAttach,volumeAttach;

    void styleKnob(juce::Slider&,juce::Label&,const juce::String&,juce::Colour);
    void styleCombo(juce::ComboBox&,juce::Colour);
    void styleToggle(juce::ToggleButton&,juce::Colour);
    void drawSection(juce::Graphics&,const juce::String&,juce::Rectangle<int>,juce::Colour,bool on=true);
    void refreshPresetList();
    void randomiseAll();
    void updateKeyboardColour();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Fraktal611AudioProcessorEditor)
};
