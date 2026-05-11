#include "PluginEditor.h"

// ── Layout-Konstanten ─────────────────────────────────────────────
static constexpr int W     = 980;
static constexpr int PBAR  = 52;   // Preset-Leiste oben
static constexpr int SHDR  = 20;   // Sektion-Header-Hoehe
static constexpr int RH    = 244;  // Hoehe einer Hauptreihe
static constexpr int KH    = 52;   // Hoehe eines Knobs
static constexpr int KW    = 52;   // Breite eines Knobs
static constexpr int GAP   = 4;
// Reihe 1
static constexpr int R1Y   = PBAR + SHDR;             // = 72
static constexpr int K1A   = R1Y + 6;
static constexpr int K2A   = K1A + KH + 8;            // = 126
static constexpr int K3A   = K2A + KH + 8;            // = 186
static constexpr int DBA   = K3A + KH + 8;            // Dropdown-Reihe in R1
// Reihe 2
static constexpr int R2Y   = R1Y + RH + SHDR + GAP;  // = 340
static constexpr int K1B   = R2Y + 6;
static constexpr int K2B   = K1B + KH + 8;
static constexpr int K3B   = K2B + KH + 8;
static constexpr int DBB   = K3B + KH + 8;
// Reihe 3  (neue Features)
static constexpr int R3H   = 164;
static constexpr int R3Y   = R2Y + RH + SHDR + GAP;  // = 608
static constexpr int K1C   = R3Y + 6;
static constexpr int K2C   = K1C + KH + 8;
static constexpr int DBC   = K2C + KH + 8;
// Scope + Keyboard
static constexpr int SCOPE_H  = 60;
static constexpr int SCOPE_Y  = R3Y + R3H + GAP;
static constexpr int KEYS_H   = 50;
static constexpr int KEYS_Y   = SCOPE_Y + SCOPE_H + GAP;
static constexpr int H        = KEYS_Y + KEYS_H + GAP;  // Total-Hoehe
// Sektionen-X (5 Spalten, je ca. 196px)
static constexpr int SW  = 196;   // Sektionsbreite
static constexpr int SX0 = 0;
static constexpr int SX1 = SW;
static constexpr int SX2 = SW*2;
static constexpr int SX3 = SW*3;
static constexpr int SX4 = W - SW;
// Knob-X-Positionen innerhalb einer Sektion (3 Knobs nebeneinander)
static constexpr int KC0 = 8;
static constexpr int KC1 = KC0 + KW + 6;
static constexpr int KC2 = KC1 + KW + 6;
static constexpr int KC3 = KC2 + KW + 6;
// Farben
static const juce::Colour COL_BG    {0xFF0D0D0D};
static const juce::Colour COL_SEC   {0xFF141414};
static const juce::Colour COL_HDR   {0xFF1A1A1A};
static const juce::Colour COL_BRDR  {0xFF2A2A2A};
static const juce::Colour COL_ACC   {0xFF38C8A0};
static const juce::Colour COL_ACC2  {0xFFE87D3E};
static const juce::Colour COL_TXT   {0xFF909090};
static const juce::Colour COL_DIM   {0xFF555555};

static void styleKnob(juce::Slider& s, juce::Colour fill=COL_ACC)
{
    s.setColour(juce::Slider::rotarySliderFillColourId, fill);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xFF252525));
}
static void styleLabel(juce::Label& l, juce::Colour c=COL_TXT)
{
    l.setFont(juce::FontOptions(9.5f));
    l.setColour(juce::Label::textColourId, c);
    l.setJustificationType(juce::Justification::centred);
}
static void styleCombo(juce::ComboBox& c)
{
    c.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF141414));
    c.setColour(juce::ComboBox::textColourId,       juce::Colour(0xFF909090));
    c.setColour(juce::ComboBox::outlineColourId,    juce::Colour(0xFF2A2A2A));
    c.setColour(juce::ComboBox::arrowColourId,      COL_ACC);
}
static void styleToggle(juce::ToggleButton& b, juce::Colour on=COL_ACC)
{
    b.setColour(juce::ToggleButton::tickColourId,     on);
    b.setColour(juce::ToggleButton::textColourId,     COL_TXT);
    b.setColour(juce::ToggleButton::tickDisabledColourId, COL_DIM);
}
static void placeKnob(juce::Component& c, juce::Rectangle<int> parent, int kx, int ky)
{
    c.setBounds(parent.getX()+kx, ky, KW, KH);
}
static void placeLbl(juce::Component& c, juce::Rectangle<int> parent, int kx, int ky, int w=KW)
{
    c.setBounds(parent.getX()+kx, ky+KH, w, 12);
}

Fraktal611AudioProcessorEditor::Fraktal611AudioProcessorEditor(Fraktal611AudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
      keyboard(p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&laf);
    setSize(W, H);
    startTimerHz(30);

    auto& apvts = processor.apvts;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // Helper: accent colour chooser
    auto accentFor=[](int wIdx)->juce::Colour{
        return (wIdx==6)?juce::Colour(0xFFE87D3E):(wIdx==7)?juce::Colour(0xFF7B68EE):COL_ACC;
    };(void)accentFor;

    // ── Preset bar ───────────────────────────────────────────────
    auto addComp=[&](juce::Component& c){addAndMakeVisible(c);};
    addComp(prevPresetBtn); prevPresetBtn.setColour(juce::TextButton::buttonColourId,juce::Colour(0xFF1E1E1E));
    addComp(nextPresetBtn); nextPresetBtn.setColour(juce::TextButton::buttonColourId,juce::Colour(0xFF1E1E1E));
    addComp(savePresetBtn); savePresetBtn.setColour(juce::TextButton::buttonColourId,juce::Colour(0xFF1E1E1E));
    addComp(loadPresetFileBtn); loadPresetFileBtn.setColour(juce::TextButton::buttonColourId,juce::Colour(0xFF1E1E1E));
    addComp(presetNameLabel); presetNameLabel.setColour(juce::Label::textColourId,COL_ACC);
    presetNameLabel.setJustificationType(juce::Justification::centred);
    prevPresetBtn.onClick=[&]{processor.presetManager.loadPrev();};
    nextPresetBtn.onClick=[&]{processor.presetManager.loadNext();};
    savePresetBtn.onClick=[&]{
        auto* aw=new juce::AlertWindow("Save Preset","Preset name:",juce::MessageBoxIconType::NoIcon);
        aw->addTextEditor("name","Init");
        aw->addButton("Save",1,juce::KeyPress(juce::KeyPress::returnKey));
        aw->addButton("Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey));
        aw->enterModalState(true,juce::ModalCallbackFunction::create([aw,this](int r){
            if(r==1){auto n=aw->getTextEditorContents("name");if(n.isNotEmpty())processor.presetManager.save(n);}
            delete aw;
        }),true);
    };
    loadPresetFileBtn.onClick=[&]{
        auto fc=std::make_shared<juce::FileChooser>("Load Preset",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),"*.fk6");
        fc->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
            [fc,this](const juce::FileChooser& c){
                auto f=c.getResult();
                if(f.existsAsFile())processor.presetManager.loadFromFile(f);
            });
    };

    addComp(panicBtn); panicBtn.setColour(juce::TextButton::buttonColourId,juce::Colour(0xFF3A1010));
    panicBtn.onClick=[&]{processor.panic();};

    // ── OSC 1 ────────────────────────────────────────────────────
    addComp(waveSelector); styleCombo(waveSelector);
    addComp(osc1VolKnob);  styleKnob(osc1VolKnob); addComp(osc1VolLabel); styleLabel(osc1VolLabel); osc1VolLabel.setText("Vol",juce::dontSendNotification);
    addComp(pwmKnob);      styleKnob(pwmKnob);      addComp(pwmLabel);     styleLabel(pwmLabel);     pwmLabel.setText("PWM",juce::dontSendNotification);
    addComp(osc1FilterCutKnob); styleKnob(osc1FilterCutKnob,COL_ACC2); addComp(osc1FilterCutLabel); styleLabel(osc1FilterCutLabel); osc1FilterCutLabel.setText("Cut",juce::dontSendNotification);
    addComp(osc1OctBox); styleCombo(osc1OctBox);
    addComp(osc1OnBtn); styleToggle(osc1OnBtn);
    addComp(osc1FilterOnBtn); styleToggle(osc1FilterOnBtn,COL_ACC2);
    addComp(osc1FilterHPBtn); styleToggle(osc1FilterHPBtn,COL_ACC2);
    waveAt=std::make_unique<CA>(apvts,"wave",waveSelector);
    osc1VolAt=std::make_unique<SA>(apvts,"osc1Vol",osc1VolKnob);
    pwmAt=std::make_unique<SA>(apvts,"pwm",pwmKnob);
    osc1FilterCutAt=std::make_unique<SA>(apvts,"osc1FilterCutoff",osc1FilterCutKnob);
    osc1OctAt=std::make_unique<CA>(apvts,"osc1Oct",osc1OctBox);
    osc1OnAt=std::make_unique<BA>(apvts,"osc1On",osc1OnBtn);
    osc1FilterOnAt=std::make_unique<BA>(apvts,"osc1FilterOn",osc1FilterOnBtn);
    osc1FilterHPAt=std::make_unique<BA>(apvts,"osc1FilterHP",osc1FilterHPBtn);

    // ── OSC 2 ────────────────────────────────────────────────────
    addComp(wave2Selector); styleCombo(wave2Selector);
    addComp(osc2VolKnob);   styleKnob(osc2VolKnob);   addComp(osc2VolLabel); styleLabel(osc2VolLabel); osc2VolLabel.setText("Vol",juce::dontSendNotification);
    addComp(osc2DetuneKnob);styleKnob(osc2DetuneKnob);addComp(osc2DetLabel); styleLabel(osc2DetLabel); osc2DetLabel.setText("Detune",juce::dontSendNotification);
    addComp(osc2PwmKnob);   styleKnob(osc2PwmKnob);   addComp(osc2PwmLabel); styleLabel(osc2PwmLabel); osc2PwmLabel.setText("PWM",juce::dontSendNotification);
    addComp(osc2FilterCutKnob); styleKnob(osc2FilterCutKnob,COL_ACC2); addComp(osc2FilterCutLabel); styleLabel(osc2FilterCutLabel); osc2FilterCutLabel.setText("Cut",juce::dontSendNotification);
    addComp(osc2OctBox); styleCombo(osc2OctBox);
    addComp(osc2OnBtn); styleToggle(osc2OnBtn);
    addComp(osc2FilterOnBtn); styleToggle(osc2FilterOnBtn,COL_ACC2);
    addComp(osc2FilterHPBtn); styleToggle(osc2FilterHPBtn,COL_ACC2);
    wave2At=std::make_unique<CA>(apvts,"wave2",wave2Selector);
    osc2VolAt=std::make_unique<SA>(apvts,"osc2Vol",osc2VolKnob);
    osc2DetuneAt=std::make_unique<SA>(apvts,"osc2Detune",osc2DetuneKnob);
    osc2PwmAt=std::make_unique<SA>(apvts,"osc2Pwm",osc2PwmKnob);
    osc2FilterCutAt=std::make_unique<SA>(apvts,"osc2FilterCutoff",osc2FilterCutKnob);
    osc2OctAt=std::make_unique<CA>(apvts,"osc2Oct",osc2OctBox);
    osc2OnAt=std::make_unique<BA>(apvts,"osc2On",osc2OnBtn);
    osc2FilterOnAt=std::make_unique<BA>(apvts,"osc2FilterOn",osc2FilterOnBtn);
    osc2FilterHPAt=std::make_unique<BA>(apvts,"osc2FilterHP",osc2FilterHPBtn);

    // ── Filter + LFO1 ────────────────────────────────────────────
    addComp(cutoffKnob);    styleKnob(cutoffKnob,COL_ACC2);    addComp(cutoffLabel);    styleLabel(cutoffLabel);    cutoffLabel.setText("Cutoff",juce::dontSendNotification);
    addComp(resonanceKnob); styleKnob(resonanceKnob,COL_ACC2); addComp(resLabel);       styleLabel(resLabel);       resLabel.setText("Res",juce::dontSendNotification);
    addComp(driveKnob);     styleKnob(driveKnob,COL_ACC2);     addComp(driveLabel);     styleLabel(driveLabel);     driveLabel.setText("Drive",juce::dontSendNotification);
    addComp(envAmtKnob);    styleKnob(envAmtKnob,COL_ACC2);    addComp(envAmtLabel);    styleLabel(envAmtLabel);    envAmtLabel.setText("Env",juce::dontSendNotification);
    addComp(lfoRateKnob);   styleKnob(lfoRateKnob);            addComp(lfoRateLabel);   styleLabel(lfoRateLabel);   lfoRateLabel.setText("Rate",juce::dontSendNotification);
    addComp(lfoDepthKnob);  styleKnob(lfoDepthKnob);           addComp(lfoDepthLabel);  styleLabel(lfoDepthLabel);  lfoDepthLabel.setText("Depth",juce::dontSendNotification);
    addComp(filterTypeBox); styleCombo(filterTypeBox);
    addComp(lfoRateMultBox);styleCombo(lfoRateMultBox);
    addComp(filterOnBtn);   styleToggle(filterOnBtn,COL_ACC2);
    addComp(fxOnBtn);       styleToggle(fxOnBtn,COL_ACC2);
    addComp(lfoMultLabel);  styleLabel(lfoMultLabel); lfoMultLabel.setText("LFO1x",juce::dontSendNotification);
    cutoffAt=std::make_unique<SA>(apvts,"cutoff",cutoffKnob);
    resonanceAt=std::make_unique<SA>(apvts,"resonance",resonanceKnob);
    driveAt=std::make_unique<SA>(apvts,"drive",driveKnob);
    envAmtAt=std::make_unique<SA>(apvts,"filterEnvAmt",envAmtKnob);
    lfoRateAt=std::make_unique<SA>(apvts,"lfoRate",lfoRateKnob);
    lfoDepthAt=std::make_unique<SA>(apvts,"lfoDepth",lfoDepthKnob);
    filterTypeAt=std::make_unique<CA>(apvts,"filterType",filterTypeBox);
    lfoRateMultAt=std::make_unique<CA>(apvts,"lfoRateMult",lfoRateMultBox);
    filterOnAt=std::make_unique<BA>(apvts,"secFilter",filterOnBtn);
    fxOnAt=std::make_unique<BA>(apvts,"secFX",fxOnBtn);

    // ── ADSR ─────────────────────────────────────────────────────
    addComp(attackKnob);  styleKnob(attackKnob);  addComp(attackLabel);  styleLabel(attackLabel);  attackLabel.setText("Attack",juce::dontSendNotification);
    addComp(decayKnob);   styleKnob(decayKnob);   addComp(decayLabel);   styleLabel(decayLabel);   decayLabel.setText("Decay",juce::dontSendNotification);
    addComp(sustainKnob); styleKnob(sustainKnob); addComp(sustainLabel); styleLabel(sustainLabel); sustainLabel.setText("Sustain",juce::dontSendNotification);
    addComp(releaseKnob); styleKnob(releaseKnob); addComp(releaseLabel); styleLabel(releaseLabel); releaseLabel.setText("Release",juce::dontSendNotification);
    attackAt=std::make_unique<SA>(apvts,"attack",attackKnob);
    decayAt=std::make_unique<SA>(apvts,"decay",decayKnob);
    sustainAt=std::make_unique<SA>(apvts,"sustain",sustainKnob);
    releaseAt=std::make_unique<SA>(apvts,"release",releaseKnob);

    // ── FX ───────────────────────────────────────────────────────
    addComp(chaosKnob);     styleKnob(chaosKnob,juce::Colour(0xFFE87D3E));     addComp(chaosLabel);     styleLabel(chaosLabel);     chaosLabel.setText("Chaos",juce::dontSendNotification);
    addComp(chaos2Knob);    styleKnob(chaos2Knob,juce::Colour(0xFFE87D3E));    addComp(chaos2Label);    styleLabel(chaos2Label);    chaos2Label.setText("Fractal",juce::dontSendNotification);
    addComp(stutterKnob);   styleKnob(stutterKnob);   addComp(stutterLabel);   styleLabel(stutterLabel);   stutterLabel.setText("Stutter",juce::dontSendNotification);
    addComp(morphKnob);     styleKnob(morphKnob);     addComp(morphLabel);     styleLabel(morphLabel);     morphLabel.setText("Morph",juce::dontSendNotification);
    addComp(morphAutoKnob); styleKnob(morphAutoKnob); addComp(morphAutoLabel); styleLabel(morphAutoLabel); morphAutoLabel.setText("Auto",juce::dontSendNotification);
    addComp(phaseDestKnob); styleKnob(phaseDestKnob); addComp(phaseDestLabel); styleLabel(phaseDestLabel); phaseDestLabel.setText("PhaseDst",juce::dontSendNotification);
    addComp(timeFoldKnob);  styleKnob(timeFoldKnob);  addComp(timeFoldLabel);  styleLabel(timeFoldLabel);  timeFoldLabel.setText("TimeFold",juce::dontSendNotification);
    addComp(stutterDivBox); styleCombo(stutterDivBox);
    chaosAt=std::make_unique<SA>(apvts,"chaos",chaosKnob);
    chaos2At=std::make_unique<SA>(apvts,"chaos2",chaos2Knob);
    stutterAt=std::make_unique<SA>(apvts,"stutter",stutterKnob);
    morphAt=std::make_unique<SA>(apvts,"morph",morphKnob);
    morphAutoAt=std::make_unique<SA>(apvts,"morphAuto",morphAutoKnob);
    phaseDestAt=std::make_unique<SA>(apvts,"phaseDest",phaseDestKnob);
    timeFoldAt=std::make_unique<SA>(apvts,"timeFold",timeFoldKnob);
    stutterDivAt=std::make_unique<CA>(apvts,"stutterDiv",stutterDivBox);

    // ── SMP 1 ────────────────────────────────────────────────────
    addComp(loadSmpBtn); loadSmpBtn.setColour(juce::TextButton::buttonColourId,juce::Colour(0xFF1E2A1E));
    loadSmpBtn.onClick=[&]{
        auto fc=std::make_shared<juce::FileChooser>("Load Sample",
            juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
            "*.wav;*.aif;*.aiff;*.flac;*.mp3");
        fc->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
            [fc,this](const juce::FileChooser& c){
                auto f=c.getResult();
                if(f.existsAsFile())processor.sharedSample.loadFile(f);
            });
    };
    addComp(smpLabel); smpLabel.setColour(juce::Label::textColourId,COL_DIM); smpLabel.setJustificationType(juce::Justification::centred);
    addComp(smpStartKnob);    styleKnob(smpStartKnob);     addComp(smpStartLabel);     styleLabel(smpStartLabel);     smpStartLabel.setText("Start",juce::dontSendNotification);
    addComp(smpEndKnob);      styleKnob(smpEndKnob);       addComp(smpEndLabel);       styleLabel(smpEndLabel);       smpEndLabel.setText("End",juce::dontSendNotification);
    addComp(osc3VolKnob);     styleKnob(osc3VolKnob);      addComp(osc3VolLabel);      styleLabel(osc3VolLabel);      osc3VolLabel.setText("Vol",juce::dontSendNotification);
    addComp(smpPitchOctKnob); styleKnob(smpPitchOctKnob,juce::Colour(0xFFE87D3E)); addComp(smpPitchOctLabel); styleLabel(smpPitchOctLabel); smpPitchOctLabel.setText("±Oct",juce::dontSendNotification);
    addComp(smpFilterCutKnob); styleKnob(smpFilterCutKnob,COL_ACC2); addComp(smpFilterCutLabel); styleLabel(smpFilterCutLabel); smpFilterCutLabel.setText("Cut",juce::dontSendNotification);
    addComp(smpRootBox); styleCombo(smpRootBox);
    addComp(smpLoopBox); styleCombo(smpLoopBox);
    addComp(osc3OnBtn); styleToggle(osc3OnBtn);
    addComp(smpFilterOnBtn); styleToggle(smpFilterOnBtn,COL_ACC2);
    smpStartAt=std::make_unique<SA>(apvts,"smpStart",smpStartKnob);
    smpEndAt=std::make_unique<SA>(apvts,"smpEnd",smpEndKnob);
    osc3VolAt=std::make_unique<SA>(apvts,"osc3Vol",osc3VolKnob);
    smpFilterCutAt=std::make_unique<SA>(apvts,"smp1FilterCutoff",smpFilterCutKnob);
    smpPitchOctAt=std::make_unique<SA>(apvts,"smpPitchOct",smpPitchOctKnob);
    smpRootAt=std::make_unique<CA>(apvts,"smpRoot",smpRootBox);
    smpLoopAt=std::make_unique<CA>(apvts,"smpLoop",smpLoopBox);
    osc3OnAt=std::make_unique<BA>(apvts,"osc3On",osc3OnBtn);
    smpFilterOnAt=std::make_unique<BA>(apvts,"smp1FilterOn",smpFilterOnBtn);

    // ── SMP 2 ────────────────────────────────────────────────────
    addComp(loadSmp2Btn); loadSmp2Btn.setColour(juce::TextButton::buttonColourId,juce::Colour(0xFF1E2A1E));
    loadSmp2Btn.onClick=[&]{
        auto fc=std::make_shared<juce::FileChooser>("Load Sample 2",
            juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
            "*.wav;*.aif;*.aiff;*.flac;*.mp3");
        fc->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
            [fc,this](const juce::FileChooser& c){
                auto f=c.getResult();
                if(f.existsAsFile())processor.getSharedSample2().loadFile(f);
            });
    };
    addComp(smp2Label); smp2Label.setColour(juce::Label::textColourId,COL_DIM); smp2Label.setJustificationType(juce::Justification::centred);
    addComp(smp2StartKnob);     styleKnob(smp2StartKnob);         addComp(smp2StartLabel);     styleLabel(smp2StartLabel);     smp2StartLabel.setText("Start",juce::dontSendNotification);
    addComp(smp2EndKnob);       styleKnob(smp2EndKnob);           addComp(smp2EndLabel);       styleLabel(smp2EndLabel);       smp2EndLabel.setText("End",juce::dontSendNotification);
    addComp(osc4VolKnob);       styleKnob(osc4VolKnob);           addComp(osc4VolLabel);       styleLabel(osc4VolLabel);       osc4VolLabel.setText("Vol",juce::dontSendNotification);
    addComp(smp2GrainRateKnob); styleKnob(smp2GrainRateKnob,juce::Colour(0xFFE87D3E)); addComp(smp2GrainRateLabel); styleLabel(smp2GrainRateLabel); smp2GrainRateLabel.setText("GrainHz",juce::dontSendNotification);
    addComp(smp2StutterAccKnob);styleKnob(smp2StutterAccKnob,juce::Colour(0xFFE87D3E)); addComp(smp2StutterAccLabel); styleLabel(smp2StutterAccLabel); smp2StutterAccLabel.setText("StAcc",juce::dontSendNotification);
    addComp(smp2PitchOctKnob);  styleKnob(smp2PitchOctKnob,juce::Colour(0xFFE87D3E));  addComp(smp2PitchOctLabel);  styleLabel(smp2PitchOctLabel);  smp2PitchOctLabel.setText("±Oct",juce::dontSendNotification);
    addComp(smp2FilterCutKnob); styleKnob(smp2FilterCutKnob,COL_ACC2); addComp(smp2FilterCutLabel); styleLabel(smp2FilterCutLabel); smp2FilterCutLabel.setText("Cut",juce::dontSendNotification);
    addComp(smp2RootBox); styleCombo(smp2RootBox);
    addComp(osc4OnBtn); styleToggle(osc4OnBtn);
    addComp(smp2FilterOnBtn); styleToggle(smp2FilterOnBtn,COL_ACC2);
    addComp(smp2RevBtn); styleToggle(smp2RevBtn,juce::Colour(0xFFE87D3E));
    smp2StartAt=std::make_unique<SA>(apvts,"smp2Start",smp2StartKnob);
    smp2EndAt=std::make_unique<SA>(apvts,"smp2End",smp2EndKnob);
    osc4VolAt=std::make_unique<SA>(apvts,"osc4Vol",osc4VolKnob);
    smp2GrainRateAt=std::make_unique<SA>(apvts,"smp2GrainRate",smp2GrainRateKnob);
    smp2StutterAccAt=std::make_unique<SA>(apvts,"smp2StutterAcc",smp2StutterAccKnob);
    smp2PitchOctAt=std::make_unique<SA>(apvts,"smp2PitchOct",smp2PitchOctKnob);
    smp2FilterCutAt=std::make_unique<SA>(apvts,"smp2FilterCutoff",smp2FilterCutKnob);
    smp2RootAt=std::make_unique<CA>(apvts,"smp2Root",smp2RootBox);
    osc4OnAt=std::make_unique<BA>(apvts,"osc4On",osc4OnBtn);
    smp2FilterOnAt=std::make_unique<BA>(apvts,"smp2FilterOn",smp2FilterOnBtn);
    smp2RevAt=std::make_unique<BA>(apvts,"smp2Reverse",smp2RevBtn);

    // ── LFO 2 ────────────────────────────────────────────────────
    addComp(lfo2RateKnob);    styleKnob(lfo2RateKnob);    addComp(lfo2RateLabel);    styleLabel(lfo2RateLabel);    lfo2RateLabel.setText("Rate",juce::dontSendNotification);
    addComp(lfo2DepthKnob);   styleKnob(lfo2DepthKnob);   addComp(lfo2DepthLabel);   styleLabel(lfo2DepthLabel);   lfo2DepthLabel.setText("Depth",juce::dontSendNotification);
    addComp(lfo2AttackKnob);  styleKnob(lfo2AttackKnob,COL_ACC2); addComp(lfo2AtkLabel); styleLabel(lfo2AtkLabel); lfo2AtkLabel.setText("A",juce::dontSendNotification);
    addComp(lfo2DecayKnob);   styleKnob(lfo2DecayKnob,COL_ACC2);  addComp(lfo2DecLabel); styleLabel(lfo2DecLabel); lfo2DecLabel.setText("D",juce::dontSendNotification);
    addComp(lfo2SustainKnob); styleKnob(lfo2SustainKnob,COL_ACC2);addComp(lfo2SusLabel); styleLabel(lfo2SusLabel); lfo2SusLabel.setText("S",juce::dontSendNotification);
    addComp(lfo2ReleaseKnob); styleKnob(lfo2ReleaseKnob,COL_ACC2);addComp(lfo2RelLabel); styleLabel(lfo2RelLabel); lfo2RelLabel.setText("R",juce::dontSendNotification);
    addComp(lfo2TargetBox);   styleCombo(lfo2TargetBox);
    addComp(lfo2RateMultBox); styleCombo(lfo2RateMultBox);
    addComp(lfo2OnBtn);       styleToggle(lfo2OnBtn);
    addComp(lfo2MultLabel);   styleLabel(lfo2MultLabel); lfo2MultLabel.setText("LFO2x",juce::dontSendNotification);
    lfo2RateAt=std::make_unique<SA>(apvts,"lfo2Rate",lfo2RateKnob);
    lfo2DepthAt=std::make_unique<SA>(apvts,"lfo2Depth",lfo2DepthKnob);
    lfo2AttackAt=std::make_unique<SA>(apvts,"lfo2Attack",lfo2AttackKnob);
    lfo2DecayAt=std::make_unique<SA>(apvts,"lfo2Decay",lfo2DecayKnob);
    lfo2SustainAt=std::make_unique<SA>(apvts,"lfo2Sustain",lfo2SustainKnob);
    lfo2ReleaseAt=std::make_unique<SA>(apvts,"lfo2Release",lfo2ReleaseKnob);
    lfo2TargetAt=std::make_unique<CA>(apvts,"lfo2Target",lfo2TargetBox);
    lfo2RateMultAt=std::make_unique<CA>(apvts,"lfo2RateMult",lfo2RateMultBox);
    lfo2OnAt=std::make_unique<BA>(apvts,"secLFO2",lfo2OnBtn);

    // ── Arpeggiator ──────────────────────────────────────────────
    addComp(arpOnBtn); styleToggle(arpOnBtn);
    addComp(arpModeBox); styleCombo(arpModeBox); addComp(arpModeLabel); styleLabel(arpModeLabel); arpModeLabel.setText("Mode",juce::dontSendNotification);
    addComp(arpDivBox);  styleCombo(arpDivBox);  addComp(arpDivLabel);  styleLabel(arpDivLabel);  arpDivLabel.setText("Div",juce::dontSendNotification);
    addComp(arpOctBox);  styleCombo(arpOctBox);  addComp(arpOctLabel);  styleLabel(arpOctLabel);  arpOctLabel.setText("Oct",juce::dontSendNotification);
    arpOnAt=std::make_unique<BA>(apvts,"arpOn",arpOnBtn);
    arpModeAt=std::make_unique<CA>(apvts,"arpMode",arpModeBox);
    arpDivAt=std::make_unique<CA>(apvts,"arpDiv",arpDivBox);
    arpOctAt=std::make_unique<CA>(apvts,"arpOctaves",arpOctBox);

    // ── Master ───────────────────────────────────────────────────
    addComp(volumeKnob);      styleKnob(volumeKnob,COL_ACC);       addComp(volumeLabel);      styleLabel(volumeLabel);      volumeLabel.setText("Volume",juce::dontSendNotification);
    addComp(delayMixKnob);    styleKnob(delayMixKnob);             addComp(delayMixLabel);    styleLabel(delayMixLabel);    delayMixLabel.setText("Dly Mix",juce::dontSendNotification);
    addComp(delayTimeKnob);   styleKnob(delayTimeKnob);            addComp(delayTimeLabel);   styleLabel(delayTimeLabel);   delayTimeLabel.setText("Dly Time",juce::dontSendNotification);
    addComp(delayFBKnob);     styleKnob(delayFBKnob);              addComp(delayFBLabel);     styleLabel(delayFBLabel);     delayFBLabel.setText("Dly FB",juce::dontSendNotification);
    addComp(reverbMixKnob);   styleKnob(reverbMixKnob,COL_ACC2);  addComp(reverbMixLabel);   styleLabel(reverbMixLabel);   reverbMixLabel.setText("Rvb Mix",juce::dontSendNotification);
    addComp(reverbSizeKnob);  styleKnob(reverbSizeKnob,COL_ACC2); addComp(reverbSizeLabel);  styleLabel(reverbSizeLabel);  reverbSizeLabel.setText("Rvb Size",juce::dontSendNotification);
    addComp(portaKnob);       styleKnob(portaKnob);                addComp(portaLabel);       styleLabel(portaLabel);       portaLabel.setText("Porta",juce::dontSendNotification);
    addComp(unisonDetuneKnob);styleKnob(unisonDetuneKnob);        addComp(uniDetLabel);      styleLabel(uniDetLabel);      uniDetLabel.setText("Unison",juce::dontSendNotification);
    addComp(gran1Knob);       styleKnob(gran1Knob,juce::Colour(0xFF7B68EE)); addComp(gran1Label); styleLabel(gran1Label); gran1Label.setText("Gran 1",juce::dontSendNotification);
    addComp(gran2Knob);       styleKnob(gran2Knob,juce::Colour(0xFF7B68EE)); addComp(gran2Label); styleLabel(gran2Label); gran2Label.setText("Gran 2",juce::dontSendNotification);
    addComp(monoBtn); styleToggle(monoBtn);
    volumeAt=std::make_unique<SA>(apvts,"volume",volumeKnob);
    delayMixAt=std::make_unique<SA>(apvts,"delayMix",delayMixKnob);
    delayTimeAt=std::make_unique<SA>(apvts,"delayTime",delayTimeKnob);
    delayFBAt=std::make_unique<SA>(apvts,"delayFeedback",delayFBKnob);
    reverbMixAt=std::make_unique<SA>(apvts,"reverbMix",reverbMixKnob);
    reverbSizeAt=std::make_unique<SA>(apvts,"reverbSize",reverbSizeKnob);
    portaAt=std::make_unique<SA>(apvts,"portamento",portaKnob);
    unisonDetuneAt=std::make_unique<SA>(apvts,"unisonDetune",unisonDetuneKnob);
    gran1At=std::make_unique<SA>(apvts,"gran1",gran1Knob);
    gran2At=std::make_unique<SA>(apvts,"gran2",gran2Knob);
    monoAt=std::make_unique<BA>(apvts,"monoMode",monoBtn);

    // ══════════════════════════════════════════════════════════════
    // REIHE 3: Neue Features
    // ══════════════════════════════════════════════════════════════

    // ── SX0: Lorenz Navigator ────────────────────────────────────
    addComp(lorenzSigmaKnob); styleKnob(lorenzSigmaKnob,juce::Colour(0xFFE87D3E)); addComp(lorenzSigmaLabel); styleLabel(lorenzSigmaLabel); lorenzSigmaLabel.setText(u8"\xcf\x83",juce::dontSendNotification);
    addComp(lorenzRhoKnob);   styleKnob(lorenzRhoKnob,  juce::Colour(0xFFE87D3E)); addComp(lorenzRhoLabel);   styleLabel(lorenzRhoLabel);   lorenzRhoLabel.setText(  u8"\xcf\x81",juce::dontSendNotification);
    addComp(lorenzBetaKnob);  styleKnob(lorenzBetaKnob, juce::Colour(0xFFE87D3E)); addComp(lorenzBetaLabel);  styleLabel(lorenzBetaLabel);  lorenzBetaLabel.setText( u8"\xce\xb2",juce::dontSendNotification);
    addComp(lorenzDtKnob);    styleKnob(lorenzDtKnob,   juce::Colour(0xFFE87D3E)); addComp(lorenzDtLabel);    styleLabel(lorenzDtLabel);    lorenzDtLabel.setText(   "dt",juce::dontSendNotification);
    lorenzSigmaAt=std::make_unique<SA>(apvts,"lorenzSigma",lorenzSigmaKnob);
    lorenzRhoAt=std::make_unique<SA>(apvts,"lorenzRho",lorenzRhoKnob);
    lorenzBetaAt=std::make_unique<SA>(apvts,"lorenzBeta",lorenzBetaKnob);
    lorenzDtAt=std::make_unique<SA>(apvts,"lorenzDt",lorenzDtKnob);

    // ── SX1: Gray-Scott ──────────────────────────────────────────
    addComp(gsOnBtn);   styleToggle(gsOnBtn,COL_ACC);
    addComp(gsFeedKnob);styleKnob(gsFeedKnob,COL_ACC); addComp(gsFeedLabel);styleLabel(gsFeedLabel);gsFeedLabel.setText("Feed",juce::dontSendNotification);
    addComp(gsKillKnob);styleKnob(gsKillKnob,COL_ACC); addComp(gsKillLabel);styleLabel(gsKillLabel);gsKillLabel.setText("Kill",juce::dontSendNotification);
    addComp(gsDaKnob);  styleKnob(gsDaKnob,COL_ACC);   addComp(gsDaLabel);  styleLabel(gsDaLabel);  gsDaLabel.setText("Da",juce::dontSendNotification);
    addComp(gsDbKnob);  styleKnob(gsDbKnob,COL_ACC);   addComp(gsDbLabel);  styleLabel(gsDbLabel);  gsDbLabel.setText("Db",juce::dontSendNotification);
    addComp(gsVolKnob); styleKnob(gsVolKnob,COL_ACC);  addComp(gsVolLabel); styleLabel(gsVolLabel); gsVolLabel.setText("Vol",juce::dontSendNotification);
    gsOnAt=std::make_unique<BA>(apvts,"gsOn",gsOnBtn);
    gsFeedAt=std::make_unique<SA>(apvts,"gsFeed",gsFeedKnob);
    gsKillAt=std::make_unique<SA>(apvts,"gsKill",gsKillKnob);
    gsDaAt=std::make_unique<SA>(apvts,"gsDa",gsDaKnob);
    gsDbAt=std::make_unique<SA>(apvts,"gsDb",gsDbKnob);
    gsVolAt=std::make_unique<SA>(apvts,"gsVol",gsVolKnob);

    // ── SX2: Weierstrass ─────────────────────────────────────────
    addComp(weiDimKnob); styleKnob(weiDimKnob,juce::Colour(0xFF7B68EE)); addComp(weiDimLabel); styleLabel(weiDimLabel); weiDimLabel.setText("Dim",juce::dontSendNotification);
    addComp(weiLacBox);  styleCombo(weiLacBox);  addComp(weiLacLabel);  styleLabel(weiLacLabel);  weiLacLabel.setText("Lacun",juce::dontSendNotification);
    addComp(weiDepthBox);styleCombo(weiDepthBox);addComp(weiDepthLabel);styleLabel(weiDepthLabel);weiDepthLabel.setText("Depth",juce::dontSendNotification);
    weiDimAt=std::make_unique<SA>(apvts,"weiDimension",weiDimKnob);
    weiLacAt=std::make_unique<CA>(apvts,"weiLacunarity",weiLacBox);
    weiDepthAt=std::make_unique<CA>(apvts,"weiDepth",weiDepthBox);

    // ── SX3: Evolutionary Drift ───────────────────────────────────
    addComp(driftOnBtn);     styleToggle(driftOnBtn,juce::Colour(0xFFFFCC00));
    addComp(driftSpeedKnob); styleKnob(driftSpeedKnob,juce::Colour(0xFFFFCC00)); addComp(driftSpeedLabel); styleLabel(driftSpeedLabel); driftSpeedLabel.setText("Speed",juce::dontSendNotification);
    addComp(driftRangeKnob); styleKnob(driftRangeKnob,juce::Colour(0xFFFFCC00)); addComp(driftRangeLabel); styleLabel(driftRangeLabel); driftRangeLabel.setText("Range",juce::dontSendNotification);
    addComp(driftModeBox);   styleCombo(driftModeBox); addComp(driftModeLabel); styleLabel(driftModeLabel); driftModeLabel.setText("Mode",juce::dontSendNotification);
    driftOnAt=std::make_unique<BA>(apvts,"driftOn",driftOnBtn);
    driftSpeedAt=std::make_unique<SA>(apvts,"driftSpeed",driftSpeedKnob);
    driftRangeAt=std::make_unique<SA>(apvts,"driftRange",driftRangeKnob);
    driftModeAt=std::make_unique<CA>(apvts,"driftMode",driftModeBox);

    // ── SX4: Poly-LFO labels ──────────────────────────────────────
    addComp(lfoMultR3Label);  styleLabel(lfoMultR3Label,COL_TXT);  lfoMultR3Label.setText("LFO1 Mult",juce::dontSendNotification);
    addComp(lfo2MultR3Label); styleLabel(lfo2MultR3Label,COL_TXT); lfo2MultR3Label.setText("LFO2 Mult",juce::dontSendNotification);

    // Keyboard
    addComp(keyboard);
    keyboard.setOctaveForMiddleC(4);
}

Fraktal611AudioProcessorEditor::~Fraktal611AudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void Fraktal611AudioProcessorEditor::timerCallback()
{
    processor.getScopeData(scopeData);
    repaint(0, SCOPE_Y, W, SCOPE_H);
    // Preset name
    presetNameLabel.setText(processor.presetManager.getCurrentName(), juce::dontSendNotification);
    // SMP labels
    smpLabel.setText(processor.sharedSample.hasFile()
        ? processor.sharedSample.getFileName() : "No File", juce::dontSendNotification);
    smp2Label.setText(processor.getSharedSample2().hasFile()
        ? processor.getSharedSample2().getFileName() : "No File", juce::dontSendNotification);
}

void Fraktal611AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(COL_BG);
    auto drawSectionBox=[&](int sx,int sy,int sw,int sh,const juce::String& title,juce::Colour accent=COL_BRDR){
        g.setColour(COL_SEC); g.fillRect(sx,sy,sw,sh);
        g.setColour(accent.withAlpha(0.25f)); g.fillRect(sx,sy,sw,SHDR);
        g.setColour(COL_BRDR.withAlpha(0.6f)); g.drawRect(sx,sy,sw,sh,1);
        g.setFont(juce::FontOptions(10.f,juce::Font::bold));
        g.setColour(accent); g.drawText(title,sx+6,sy,sw-8,SHDR,juce::Justification::centredLeft);
    };
    // Row 1 headers
    drawSectionBox(SX0,PBAR,SW,RH+SHDR,"OSC 1",COL_ACC);
    drawSectionBox(SX1,PBAR,SW,RH+SHDR,"OSC 2",COL_ACC);
    drawSectionBox(SX2,PBAR,SW,RH+SHDR,"FILTER + LFO",COL_ACC2);
    drawSectionBox(SX3,PBAR,SW,RH+SHDR,"ADSR",COL_ACC);
    drawSectionBox(SX4,PBAR,W-SX4,RH+SHDR,"FX",juce::Colour(0xFFE87D3E));
    // Row 2 headers
    drawSectionBox(SX0,R2Y-SHDR,SW,RH+SHDR,"SMP 1",COL_ACC);
    drawSectionBox(SX1,R2Y-SHDR,SW,RH+SHDR,"SMP 2",juce::Colour(0xFFE87D3E));
    drawSectionBox(SX2,R2Y-SHDR,SW,RH+SHDR,"LFO 2",COL_ACC);
    drawSectionBox(SX3,R2Y-SHDR,SW,RH+SHDR,"ARPEGG",COL_ACC);
    drawSectionBox(SX4,R2Y-SHDR,W-SX4,RH+SHDR,"MASTER",COL_ACC);
    // Row 3 headers (neue Features)
    drawSectionBox(SX0,R3Y-SHDR,SW,R3H+SHDR,"LORENZ NAV",juce::Colour(0xFFE87D3E));
    drawSectionBox(SX1,R3Y-SHDR,SW,R3H+SHDR,"GRAY-SCOTT RD",COL_ACC);
    drawSectionBox(SX2,R3Y-SHDR,SW,R3H+SHDR,"WEIERSTRASS",juce::Colour(0xFF7B68EE));
    drawSectionBox(SX3,R3Y-SHDR,SW,R3H+SHDR,"EVOL. DRIFT",juce::Colour(0xFFFFCC00));
    drawSectionBox(SX4,R3Y-SHDR,W-SX4,R3H+SHDR,"POLY-TEMPORAL LFO",juce::Colour(0xFF7B68EE));
    // Preset bar
    g.setColour(juce::Colour(0xFF181818));
    g.fillRect(0,0,W,PBAR);
    g.setColour(juce::Colour(0xFF38C8A0).withAlpha(0.5f));
    g.fillRect(0,PBAR-1,W,1);
    g.setFont(juce::FontOptions("Helvetica Neue",18.f,juce::Font::bold));
    g.setColour(COL_ACC); g.drawText("FRAKTAL 611",8,6,200,28,juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(9.f));
    g.setColour(COL_DIM); g.drawText("Fraktal-Synthesizer | Wave 6=LorenzLive | Wave 7=Weierstrass | GS-RD | Drift | Poly-LFO",8,28,W-16,16,juce::Justification::centredLeft);
    // Oscilloscope
    g.setColour(juce::Colour(0xFF0A0A0A));
    g.fillRect(0,SCOPE_Y,W,SCOPE_H);
    g.setColour(COL_BRDR.withAlpha(0.4f));
    g.drawLine(0,(float)(SCOPE_Y+SCOPE_H/2),W,(float)(SCOPE_Y+SCOPE_H/2),1.f);
    {
        juce::Path p; bool first=true;
        for(int i=0;i<(int)scopeData.size();++i){
            float x=(float)i*W/(float)scopeData.size();
            float y=SCOPE_Y+SCOPE_H/2-(scopeData[i]*SCOPE_H*0.42f);
            if(first){p.startNewSubPath(x,y);first=false;}else p.lineTo(x,y);
        }
        g.setColour(COL_ACC.withAlpha(0.8f));
        g.strokePath(p,juce::PathStrokeType(1.2f));
    }
    // SX4 Row3: Poly-LFO info text
    {
        g.setFont(juce::FontOptions(8.5f));
        g.setColour(COL_DIM);
        const char* lines[]={"Irrationale Multiplikatoren:","×1  ×φ(1.618)  ×π(3.14)","×√2(1.414)  ×√3(1.732)  ×e(2.718)","Zwei LFOs niemals synchron"};
        for(int i=0;i<4;++i) g.drawText(lines[i],SX4+6,K1C+i*14,W-SX4-8,13,juce::Justification::centredLeft);
    }
}

void Fraktal611AudioProcessorEditor::resized()
{
    // ── Preset bar ───────────────────────────────────────────────
    prevPresetBtn.setBounds(240,8,28,18);
    presetNameLabel.setBounds(272,8,200,18);
    nextPresetBtn.setBounds(476,8,28,18);
    savePresetBtn.setBounds(512,8,50,18);
    loadPresetFileBtn.setBounds(566,8,50,18);
    panicBtn.setBounds(W-58,8,50,18);

    // ── ROW 1 ────────────────────────────────────────────────────
    // OSC 1
    {
        auto s=juce::Rectangle<int>(SX0,0,SW,H);
        osc1OnBtn.setBounds(s.getX()+4,K1A,32,16);
        waveSelector.setBounds(s.getX()+38,K1A,s.getWidth()-44,18);
        osc1OctBox.setBounds(s.getX()+4,K1A+20,54,16);
        osc1FilterOnBtn.setBounds(s.getX()+60,K1A+20,24,16);
        osc1FilterHPBtn.setBounds(s.getX()+86,K1A+20,24,16);
        placeKnob(osc1VolKnob,s,KC0,K2A);  placeLbl(osc1VolLabel,s,KC0,K2A);
        placeKnob(pwmKnob,s,KC1,K2A);       placeLbl(pwmLabel,s,KC1,K2A);
        placeKnob(osc1FilterCutKnob,s,KC2,K2A); placeLbl(osc1FilterCutLabel,s,KC2,K2A);
    }
    // OSC 2
    {
        auto s=juce::Rectangle<int>(SX1,0,SW,H);
        osc2OnBtn.setBounds(s.getX()+4,K1A,32,16);
        wave2Selector.setBounds(s.getX()+38,K1A,s.getWidth()-44,18);
        osc2OctBox.setBounds(s.getX()+4,K1A+20,54,16);
        osc2FilterOnBtn.setBounds(s.getX()+60,K1A+20,24,16);
        osc2FilterHPBtn.setBounds(s.getX()+86,K1A+20,24,16);
        placeKnob(osc2VolKnob,s,KC0,K2A);       placeLbl(osc2VolLabel,s,KC0,K2A);
        placeKnob(osc2DetuneKnob,s,KC1,K2A);    placeLbl(osc2DetLabel,s,KC1,K2A);
        placeKnob(osc2PwmKnob,s,KC2,K2A);       placeLbl(osc2PwmLabel,s,KC2,K2A);
        placeKnob(osc2FilterCutKnob,s,KC0,K3A); placeLbl(osc2FilterCutLabel,s,KC0,K3A);
    }
    // Filter + LFO
    {
        auto s=juce::Rectangle<int>(SX2,0,SW,H);
        filterOnBtn.setBounds(s.getX()+4,K1A,36,16);
        fxOnBtn.setBounds(s.getX()+44,K1A,36,16);
        filterTypeBox.setBounds(s.getX()+82,K1A,s.getWidth()-88,18);
        placeKnob(cutoffKnob,s,KC0,K2A);    placeLbl(cutoffLabel,s,KC0,K2A);
        placeKnob(resonanceKnob,s,KC1,K2A); placeLbl(resLabel,s,KC1,K2A);
        placeKnob(driveKnob,s,KC2,K2A);     placeLbl(driveLabel,s,KC2,K2A);
        placeKnob(envAmtKnob,s,KC0,K3A);    placeLbl(envAmtLabel,s,KC0,K3A);
        placeKnob(lfoRateKnob,s,KC1,K3A);   placeLbl(lfoRateLabel,s,KC1,K3A);
        placeKnob(lfoDepthKnob,s,KC2,K3A);  placeLbl(lfoDepthLabel,s,KC2,K3A);
        lfoMultLabel.setBounds(s.getX()+KC0,DBA,38,12);
        lfoRateMultBox.setBounds(s.getX()+KC0+40,DBA,s.getWidth()-KC0-46,18);
    }
    // ADSR
    {
        auto s=juce::Rectangle<int>(SX3,0,SW,H);
        placeKnob(attackKnob,s,KC0,K1A);  placeLbl(attackLabel,s,KC0,K1A);
        placeKnob(decayKnob,s,KC1,K1A);   placeLbl(decayLabel,s,KC1,K1A);
        placeKnob(sustainKnob,s,KC2,K1A); placeLbl(sustainLabel,s,KC2,K1A);
        placeKnob(releaseKnob,s,KC0,K2A); placeLbl(releaseLabel,s,KC0,K2A);
    }
    // FX
    {
        auto s=juce::Rectangle<int>(SX4,0,W-SX4,H);
        placeKnob(chaosKnob,s,KC0,K1A);      placeLbl(chaosLabel,s,KC0,K1A);
        placeKnob(chaos2Knob,s,KC1,K1A);     placeLbl(chaos2Label,s,KC1,K1A);
        placeKnob(stutterKnob,s,KC0,K2A);    placeLbl(stutterLabel,s,KC0,K2A);
        placeKnob(morphKnob,s,KC1,K2A);      placeLbl(morphLabel,s,KC1,K2A);
        placeKnob(morphAutoKnob,s,KC2,K2A);  placeLbl(morphAutoLabel,s,KC2,K2A);
        placeKnob(phaseDestKnob,s,KC0,K3A);  placeLbl(phaseDestLabel,s,KC0,K3A);
        placeKnob(timeFoldKnob,s,KC1,K3A);   placeLbl(timeFoldLabel,s,KC1,K3A);
        stutterDivBox.setBounds(s.getX()+KC0,DBA,s.getWidth()-KC0-6,18);
    }

    // ── ROW 2 ────────────────────────────────────────────────────
    // SMP 1
    {
        auto s=juce::Rectangle<int>(SX0,0,SW,H);
        osc3OnBtn.setBounds(s.getX()+4,K1B,32,16);
        loadSmpBtn.setBounds(s.getX()+40,K1B,54,16);
        smpFilterOnBtn.setBounds(s.getX()+98,K1B,24,16);
        smpLabel.setBounds(s.getX()+4,K1B+20,s.getWidth()-8,12);
        placeKnob(smpStartKnob,s,KC0,K2B);     placeLbl(smpStartLabel,s,KC0,K2B);
        placeKnob(smpEndKnob,s,KC1,K2B);       placeLbl(smpEndLabel,s,KC1,K2B);
        placeKnob(osc3VolKnob,s,KC2,K2B);      placeLbl(osc3VolLabel,s,KC2,K2B);
        placeKnob(smpPitchOctKnob,s,KC0,K3B);  placeLbl(smpPitchOctLabel,s,KC0,K3B);
        placeKnob(smpFilterCutKnob,s,KC1,K3B); placeLbl(smpFilterCutLabel,s,KC1,K3B);
        smpRootBox.setBounds(s.getX()+KC0,DBB,68,18);
        smpLoopBox.setBounds(s.getX()+KC0+72,DBB,s.getWidth()-KC0-78,18);
    }
    // SMP 2
    {
        auto s=juce::Rectangle<int>(SX1,0,SW,H);
        osc4OnBtn.setBounds(s.getX()+4,K1B,32,16);
        loadSmp2Btn.setBounds(s.getX()+40,K1B,54,16);
        smp2FilterOnBtn.setBounds(s.getX()+98,K1B,24,16);
        smp2RevBtn.setBounds(s.getX()+124,K1B,32,16);
        smp2Label.setBounds(s.getX()+4,K1B+20,s.getWidth()-8,12);
        placeKnob(smp2StartKnob,s,KC0,K2B);       placeLbl(smp2StartLabel,s,KC0,K2B);
        placeKnob(smp2EndKnob,s,KC1,K2B);         placeLbl(smp2EndLabel,s,KC1,K2B);
        placeKnob(osc4VolKnob,s,KC2,K2B);         placeLbl(osc4VolLabel,s,KC2,K2B);
        placeKnob(smp2GrainRateKnob,s,KC0,K3B);   placeLbl(smp2GrainRateLabel,s,KC0,K3B);
        placeKnob(smp2StutterAccKnob,s,KC1,K3B);  placeLbl(smp2StutterAccLabel,s,KC1,K3B);
        placeKnob(smp2PitchOctKnob,s,KC2,K3B);    placeLbl(smp2PitchOctLabel,s,KC2,K3B);
        placeKnob(smp2FilterCutKnob,s,KC0,DBB);   placeLbl(smp2FilterCutLabel,s,KC0,DBB);
        smp2RootBox.setBounds(s.getX()+KC1+4,DBB,s.getWidth()-KC1-10,18);
    }
    // LFO 2
    {
        auto s=juce::Rectangle<int>(SX2,0,SW,H);
        lfo2OnBtn.setBounds(s.getX()+4,K1B,32,16);
        lfo2TargetBox.setBounds(s.getX()+38,K1B,s.getWidth()-44,18);
        placeKnob(lfo2RateKnob,s,KC0,K2B);    placeLbl(lfo2RateLabel,s,KC0,K2B);
        placeKnob(lfo2DepthKnob,s,KC1,K2B);   placeLbl(lfo2DepthLabel,s,KC1,K2B);
        placeKnob(lfo2AttackKnob,s,KC0,K3B);  placeLbl(lfo2AtkLabel,s,KC0,K3B);
        placeKnob(lfo2DecayKnob,s,KC1,K3B);   placeLbl(lfo2DecLabel,s,KC1,K3B);
        placeKnob(lfo2SustainKnob,s,KC2,K3B); placeLbl(lfo2SusLabel,s,KC2,K3B);
        lfo2MultLabel.setBounds(s.getX()+KC0,DBB,38,12);
        lfo2RateMultBox.setBounds(s.getX()+KC0+40,DBB,s.getWidth()-KC0-46,18);
    }
    // Arp
    {
        auto s=juce::Rectangle<int>(SX3,0,SW,H);
        arpOnBtn.setBounds(s.getX()+4,K1B,32,16);
        arpModeLabel.setBounds(s.getX()+4,K2B,40,12);
        arpModeBox.setBounds(s.getX()+4,K2B+12,s.getWidth()-8,18);
        arpDivLabel.setBounds(s.getX()+4,K3B,40,12);
        arpDivBox.setBounds(s.getX()+4,K3B+12,s.getWidth()/2-6,18);
        arpOctLabel.setBounds(s.getX()+s.getWidth()/2+2,K3B,40,12);
        arpOctBox.setBounds(s.getX()+s.getWidth()/2+2,K3B+12,s.getWidth()/2-8,18);
    }
    // Master
    {
        auto s=juce::Rectangle<int>(SX4,0,W-SX4,H);
        placeKnob(volumeKnob,s,KC0,K1B);      placeLbl(volumeLabel,s,KC0,K1B);
        placeKnob(portaKnob,s,KC1,K1B);       placeLbl(portaLabel,s,KC1,K1B);
        placeKnob(unisonDetuneKnob,s,KC2,K1B);placeLbl(uniDetLabel,s,KC2,K1B);
        placeKnob(delayMixKnob,s,KC0,K2B);    placeLbl(delayMixLabel,s,KC0,K2B);
        placeKnob(delayTimeKnob,s,KC1,K2B);   placeLbl(delayTimeLabel,s,KC1,K2B);
        placeKnob(delayFBKnob,s,KC2,K2B);     placeLbl(delayFBLabel,s,KC2,K2B);
        placeKnob(reverbMixKnob,s,KC0,K3B);   placeLbl(reverbMixLabel,s,KC0,K3B);
        placeKnob(reverbSizeKnob,s,KC1,K3B);  placeLbl(reverbSizeLabel,s,KC1,K3B);
        placeKnob(gran1Knob,s,KC0,DBB);       placeLbl(gran1Label,s,KC0,DBB);
        placeKnob(gran2Knob,s,KC1,DBB);       placeLbl(gran2Label,s,KC1,DBB);
        monoBtn.setBounds(s.getX()+KC2,DBB,KW,16);
    }

    // ── ROW 3: neue Features ──────────────────────────────────────
    // SX0: Lorenz Navigator
    {
        auto s=juce::Rectangle<int>(SX0,0,SW,H);
        placeKnob(lorenzSigmaKnob,s,KC0,K1C); placeLbl(lorenzSigmaLabel,s,KC0,K1C);
        placeKnob(lorenzRhoKnob,s,KC1,K1C);   placeLbl(lorenzRhoLabel,s,KC1,K1C);
        placeKnob(lorenzBetaKnob,s,KC2,K1C);  placeLbl(lorenzBetaLabel,s,KC2,K1C);
        placeKnob(lorenzDtKnob,s,KC0,K2C);    placeLbl(lorenzDtLabel,s,KC0,K2C);
    }
    // SX1: Gray-Scott
    {
        auto s=juce::Rectangle<int>(SX1,0,SW,H);
        gsOnBtn.setBounds(s.getX()+4,K1C,48,16);
        placeKnob(gsFeedKnob,s,KC0,K1C+20); placeLbl(gsFeedLabel,s,KC0,K1C+20);
        placeKnob(gsKillKnob,s,KC1,K1C+20); placeLbl(gsKillLabel,s,KC1,K1C+20);
        placeKnob(gsDaKnob,s,KC2,K1C+20);   placeLbl(gsDaLabel,s,KC2,K1C+20);
        placeKnob(gsDbKnob,s,KC0,K2C);      placeLbl(gsDbLabel,s,KC0,K2C);
        placeKnob(gsVolKnob,s,KC1,K2C);     placeLbl(gsVolLabel,s,KC1,K2C);
    }
    // SX2: Weierstrass
    {
        auto s=juce::Rectangle<int>(SX2,0,SW,H);
        placeKnob(weiDimKnob,s,KC0,K1C); placeLbl(weiDimLabel,s,KC0,K1C);
        weiLacLabel.setBounds(s.getX()+KC1,K1C,KW,12);
        weiLacBox.setBounds(s.getX()+KC1,K1C+12,KW,18);
        weiDepthLabel.setBounds(s.getX()+KC2,K1C,KW,12);
        weiDepthBox.setBounds(s.getX()+KC2,K1C+12,KW,18);
    }
    // SX3: Evolutionary Drift
    {
        auto s=juce::Rectangle<int>(SX3,0,SW,H);
        driftOnBtn.setBounds(s.getX()+4,K1C,50,16);
        placeKnob(driftSpeedKnob,s,KC0,K1C+20); placeLbl(driftSpeedLabel,s,KC0,K1C+20);
        placeKnob(driftRangeKnob,s,KC1,K1C+20); placeLbl(driftRangeLabel,s,KC1,K1C+20);
        driftModeLabel.setBounds(s.getX()+KC0,K2C,KW*2+6,12);
        driftModeBox.setBounds(s.getX()+KC0,K2C+12,KW*2+6,18);
    }
    // SX4: Poly-LFO (Info-Text + Labels; Combos sind in Filter/LFO2)
    {
        auto s=juce::Rectangle<int>(SX4,0,W-SX4,H);
        lfoMultR3Label.setBounds(s.getX()+KC0,K1C+56,s.getWidth()-KC0-6,12);
        lfo2MultR3Label.setBounds(s.getX()+KC0,K1C+72,s.getWidth()-KC0-6,12);
    }

    // Keyboard
    keyboard.setBounds(0, KEYS_Y, W, KEYS_H);
}

juce::AudioProcessorEditor* Fraktal611AudioProcessor::createEditor()
{ return new Fraktal611AudioProcessorEditor(*this); }
