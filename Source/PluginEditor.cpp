#include "PluginEditor.h"

// ── Grid Layout ───────────────────────────────────────────────────
// 980 × 680px — 5 × 196px, 2 Reihen à 240px + Scope
static constexpr int W    = 980;
static constexpr int H    = 960;   // Row 3 hinzugefügt   // höher für Keyboard
static constexpr int SEC  = 196;
static constexpr int PAD  = 8;
static constexpr int KW   = 44;
static constexpr int KH   = 50;
static constexpr int KG   = 5;
static constexpr int PBAR = 52;
static constexpr int SHDR = 20;
static constexpr int RH   = 244;
static constexpr int R1Y  = PBAR + SHDR;
static constexpr int R2Y  = R1Y + RH + SHDR + 4;
static constexpr int K1   = R1Y + 4;
static constexpr int K2   = K1 + KH + 16;
static constexpr int K3   = K2 + KH + 16;
static constexpr int K1B  = R2Y + 4;
static constexpr int K2B  = K1B + KH + 16;
static constexpr int K3B  = K2B + KH + 16;
static constexpr int DB1  = K3 + KH + 10;
static constexpr int DB2  = K3B + KH + 24;  // mehr Luft nach unten
// Row 3 (neue Features)
static constexpr int R3Y   = R2Y + RH + SHDR + 4;  // = 608
static constexpr int R3H   = 156;
static constexpr int K1C   = R3Y + 4;               // = 612
static constexpr int K2C   = K1C + KH + 16;         // = 678
static constexpr int DB3   = K2C + KH + 10;         // = 738
static constexpr int SCOPE_Y = R3Y + R3H + 6;       // = 770
static constexpr int SCOPE_H = 60;
static constexpr int KEYS_Y  = SCOPE_Y + SCOPE_H + 4;
static constexpr int KEYS_H  = H - KEYS_Y - 4;

static constexpr int SX0=0, SX1=SEC, SX2=SEC*2, SX3=SEC*3, SX4=SEC*4;
static int kx(int sec,int n){ return sec+PAD+n*(KW+KG); }

static const juce::StringArray WAVE_NAMES{"Lorenz","FM","Granular","Saw","Square","Drift"};

Fraktal611AudioProcessorEditor::Fraktal611AudioProcessorEditor
    (Fraktal611AudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&laf);
    setSize(W,H);
    auto& apvts=processor.apvts;
    addAndMakeVisible(scope);

    // ── MIDI Keyboard ──────────────────────────────────────────────
    keyboard.setAvailableRange(36, 96);   // C2–C7
    keyboard.setScrollButtonsVisible(false);
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId,
                       juce::Colour(0xFF1A1A1A));
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId,
                       juce::Colour(0xFF1A1A1A));
    keyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId,
                       juce::Colour(0xFF2A2A2A));
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                       juce::Colour(0x33C8FF00));
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,
                       juce::Colour(0xAAC8FF00));
    addAndMakeVisible(keyboard);
    startTimerHz(10); // Farb-Update 10x/Sek, leicht genug

    const juce::Colour green(0xFFC8FF00),orange(0xFFFF8C00),pink(0xFFFF4488);
    const juce::Colour cyan(0xFF00DDCC),yellow(0xFFFFDD00),violet(0xFFCC44FF);
    const juce::Colour teal(0xFF00FFCC),blue(0xFF44AAFF),red(0xFFFF5555);
    const juce::Colour lime(0xFF88FF44),smpC(0xFFFF9944),smp2C(0xFFFFCC44);

    // ── Preset Bar ────────────────────────────────────────────────
    for(int i=0;i<PresetManager::getCategories().size();++i)
        presetCategoryBox.addItem(PresetManager::getCategories()[i],i+1);
    styleCombo(presetCategoryBox,green);
    presetCategoryBox.setSelectedItemIndex(0);
    addAndMakeVisible(presetCategoryBox);
    presetCategoryBox.onChange=[this]{refreshPresetList();};
    styleCombo(presetNameBox,juce::Colour(0xFF888888));
    addAndMakeVisible(presetNameBox);
    presetNameEdit.setMultiLine(false); presetNameEdit.setText("MyPreset");
    presetNameEdit.setColour(juce::TextEditor::backgroundColourId,juce::Colour(0xFF141414));
    presetNameEdit.setColour(juce::TextEditor::textColourId,green);
    presetNameEdit.setColour(juce::TextEditor::outlineColourId,juce::Colour(0xFF2A2A2A));
    presetNameEdit.setFont(juce::FontOptions("Helvetica Neue",9.f,juce::Font::plain));
    addAndMakeVisible(presetNameEdit);
    auto styleBtn=[](juce::TextButton& b,juce::Colour c){
        b.setColour(juce::TextButton::buttonColourId,juce::Colour(0xFF181818));
        b.setColour(juce::TextButton::textColourOffId,c);
    };
    styleBtn(presetSaveBtn,green); styleBtn(presetLoadBtn,cyan); styleBtn(randomBtn,pink);
    addAndMakeVisible(presetSaveBtn); addAndMakeVisible(presetLoadBtn); addAndMakeVisible(randomBtn);
    presetSaveBtn.onClick=[this]{
        auto cat=PresetManager::getCategories()[presetCategoryBox.getSelectedItemIndex()];
        auto name=presetNameEdit.getText().trim(); if(name.isEmpty())name="Preset";
        juce::String sp=processor.getSharedSample().hasFile()?processor.getSharedSample().getFilePath():juce::String{};
        if(PresetManager::savePreset(processor.apvts,cat,name,sp)){
            refreshPresetList();
            auto ps=PresetManager::getPresetsInCategory(cat);
            int idx=ps.indexOf(name); if(idx>=0) presetNameBox.setSelectedItemIndex(idx);
        }
    };
    presetLoadBtn.onClick=[this]{
        auto cat=PresetManager::getCategories()[presetCategoryBox.getSelectedItemIndex()];
        auto name=presetNameBox.getText(); if(name.isEmpty()) return;
        auto sp=PresetManager::loadPreset(processor.apvts,cat,name);
        if(sp.isNotEmpty()){
            juce::File f(sp);
            if(f.existsAsFile()){ processor.getSharedSample().loadFile(f); smpFileLabel.setText(f.getFileName(),juce::dontSendNotification); }
            else smpFileLabel.setText("! not found: "+f.getFileName(),juce::dontSendNotification);
        } else smpFileLabel.setText("-- no sample --",juce::dontSendNotification);
    };
    randomBtn.onClick=[this]{ randomiseAll(); };

    // ══ ROW 3 Setup ══════════════════════════════════════════════
    {
        auto& a=processor.apvts;
        const juce::Colour lC(0xFFE87D3E),gC(0xFF38C8A0),wC(0xFF7B68EE),dC(0xFFFFCC00),pC(0xFF7B68EE);
        styleKnob(lorenzSigmaKnob,lorenzSigmaLabel,u8"σ",lC);
        styleKnob(lorenzRhoKnob,  lorenzRhoLabel,  u8"ρ",lC);
        styleKnob(lorenzBetaKnob, lorenzBetaLabel, u8"β",lC);
        styleKnob(lorenzDtKnob,   lorenzDtLabel,   "dt",      lC);
        lorenzSigmaAttach=std::make_unique<SA>(a,"lorenzSigma",lorenzSigmaKnob);
        lorenzRhoAttach  =std::make_unique<SA>(a,"lorenzRho",  lorenzRhoKnob);
        lorenzBetaAttach =std::make_unique<SA>(a,"lorenzBeta", lorenzBetaKnob);
        lorenzDtAttach   =std::make_unique<SA>(a,"lorenzDt",   lorenzDtKnob);
        styleToggle(gsOnBtn,gC);
        styleKnob(gsFeedKnob,gsFeedLabel,"Feed",gC); styleKnob(gsKillKnob,gsKillLabel,"Kill",gC);
        styleKnob(gsDaKnob,  gsDaLabel,  "Da",  gC); styleKnob(gsDbKnob,  gsDbLabel,  "Db",  gC);
        styleKnob(gsVolKnob, gsVolLabel, "Vol",  gC);
        gsOnAttach   =std::make_unique<BA>(a,"gsOn",  gsOnBtn);
        gsFeedAttach =std::make_unique<SA>(a,"gsFeed",gsFeedKnob); gsKillAttach=std::make_unique<SA>(a,"gsKill",gsKillKnob);
        gsDaAttach   =std::make_unique<SA>(a,"gsDa",  gsDaKnob);   gsDbAttach  =std::make_unique<SA>(a,"gsDb",  gsDbKnob);
        gsVolAttach  =std::make_unique<SA>(a,"gsVol", gsVolKnob);
        styleKnob(weiDimKnob,weiDimLabel,"Dim",wC);
        weiDimAttach=std::make_unique<SA>(a,"weiDimension",weiDimKnob);
        styleCombo(weiLacBox,wC);   addAndMakeVisible(weiLacBox);
        styleCombo(weiDepthBox,wC); addAndMakeVisible(weiDepthBox);
        if(auto*p=dynamic_cast<juce::AudioParameterChoice*>(a.getParameter("weiLacunarity")))weiLacBox.addItemList(p->choices,1);
        if(auto*p=dynamic_cast<juce::AudioParameterChoice*>(a.getParameter("weiDepth")))    weiDepthBox.addItemList(p->choices,1);
        weiLacAttach  =std::make_unique<CA>(a,"weiLacunarity",weiLacBox);
        weiDepthAttach=std::make_unique<CA>(a,"weiDepth",     weiDepthBox);
        styleToggle(driftOnBtn,dC);
        styleKnob(driftSpeedKnob,driftSpeedLabel,"Speed",dC);
        styleKnob(driftRangeKnob,driftRangeLabel,"Range",dC);
        styleCombo(driftModeBox,dC); addAndMakeVisible(driftModeBox);
        if(auto*p=dynamic_cast<juce::AudioParameterChoice*>(a.getParameter("driftMode")))driftModeBox.addItemList(p->choices,1);
        driftOnAttach   =std::make_unique<BA>(a,"driftOn",   driftOnBtn);
        driftSpeedAttach=std::make_unique<SA>(a,"driftSpeed",driftSpeedKnob);
        driftRangeAttach=std::make_unique<SA>(a,"driftRange",driftRangeKnob);
        driftModeAttach =std::make_unique<CA>(a,"driftMode", driftModeBox);
        styleCombo(lfoMultBox, pC); addAndMakeVisible(lfoMultBox);
        styleCombo(lfo2MultBox,pC); addAndMakeVisible(lfo2MultBox);
        if(auto*p=dynamic_cast<juce::AudioParameterChoice*>(a.getParameter("lfoRateMult"))) lfoMultBox.addItemList(p->choices,1);
        if(auto*p=dynamic_cast<juce::AudioParameterChoice*>(a.getParameter("lfo2RateMult")))lfo2MultBox.addItemList(p->choices,1);
        lfoMultAttach =std::make_unique<CA>(a,"lfoRateMult", lfoMultBox);
        lfo2MultAttach=std::make_unique<CA>(a,"lfo2RateMult",lfo2MultBox);
        lfoMultLabel.setText("LFO1×",juce::dontSendNotification);
        lfo2MultLabel.setText("LFO2×",juce::dontSendNotification);
        for(auto* lb:{&lfoMultLabel,&lfo2MultLabel}){lb->setColour(juce::Label::textColourId,pC);lb->setFont(juce::FontOptions(8.5f));addAndMakeVisible(lb);}
    }

    refreshPresetList();

    // ── Toggles ───────────────────────────────────────────────────
    styleToggle(osc1OnBtn,  green);  styleToggle(osc2OnBtn,  lime);
    styleToggle(osc3OnBtn,  smpC);   styleToggle(osc4OnBtn,  smp2C);
    styleToggle(osc1FOnBtn, orange); styleToggle(osc1FHPBtn, orange);
    styleToggle(osc2FOnBtn, orange); styleToggle(osc2FHPBtn, orange);
    styleToggle(smp1FOnBtn, orange); styleToggle(smp2FOnBtn, orange);
    styleToggle(secFilterBtn,green); styleToggle(secFXBtn,   pink);
    styleToggle(secLFO2Btn,  blue);  styleToggle(arpOnBtn,   cyan);
    styleToggle(monoBtn,     yellow);
    addAndMakeVisible(monoBtn);

    // PANIC: alle Noten + Delay/Reverb stoppen
    panicBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xFF3A0000));
    panicBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFFF3333));
    panicBtn.onClick=[this]{ processor.panic(); };
    addAndMakeVisible(panicBtn);

    osc1OnAttach    =std::make_unique<BA>(apvts,"osc1On",    osc1OnBtn);
    osc2OnAttach    =std::make_unique<BA>(apvts,"osc2On",    osc2OnBtn);
    osc3OnAttach    =std::make_unique<BA>(apvts,"osc3On",    osc3OnBtn);
    osc4OnAttach    =std::make_unique<BA>(apvts,"osc4On",    osc4OnBtn);
    osc1FOnAttach   =std::make_unique<BA>(apvts,"osc1FilterOn",osc1FOnBtn);
    osc1FHPAttach   =std::make_unique<BA>(apvts,"osc1FilterHP",osc1FHPBtn);
    osc2FOnAttach   =std::make_unique<BA>(apvts,"osc2FilterOn",osc2FOnBtn);
    osc2FHPAttach   =std::make_unique<BA>(apvts,"osc2FilterHP",osc2FHPBtn);
    smp1FOnAttach   =std::make_unique<BA>(apvts,"smp1FilterOn",smp1FOnBtn);
    smp2FOnAttach   =std::make_unique<BA>(apvts,"smp2FilterOn",smp2FOnBtn);
    secFilterAttach =std::make_unique<BA>(apvts,"secFilter",secFilterBtn);
    secFXAttach     =std::make_unique<BA>(apvts,"secFX",    secFXBtn);
    secLFO2Attach   =std::make_unique<BA>(apvts,"secLFO2",  secLFO2Btn);
    arpOnAttach     =std::make_unique<BA>(apvts,"arpOn",    arpOnBtn);
    monoAttach      =std::make_unique<BA>(apvts,"monoMode", monoBtn);

    // ── ComboBoxes ────────────────────────────────────────────────
    for(int i=0;i<WAVE_NAMES.size();++i) waveSelector.addItem(WAVE_NAMES[i],i+1);
    styleCombo(waveSelector,green); addAndMakeVisible(waveSelector);
    waveAttach=std::make_unique<CA>(apvts,"wave",waveSelector);

    for(int i=0;i<WAVE_NAMES.size();++i) wave2Selector.addItem(WAVE_NAMES[i],i+1);
    styleCombo(wave2Selector,lime); addAndMakeVisible(wave2Selector);
    wave2Attach=std::make_unique<CA>(apvts,"wave2",wave2Selector);

    for(auto& s:juce::StringArray{"-2","-1","0","+1","+2"}){
        osc1OctBox.addItem(s,osc1OctBox.getNumItems()+1);
        osc2OctBox.addItem(s,osc2OctBox.getNumItems()+1);
    }
    styleCombo(osc1OctBox,green); addAndMakeVisible(osc1OctBox);
    osc1OctAttach=std::make_unique<CA>(apvts,"osc1Oct",osc1OctBox);
    styleCombo(osc2OctBox,lime);  addAndMakeVisible(osc2OctBox);
    osc2OctAttach=std::make_unique<CA>(apvts,"osc2Oct",osc2OctBox);

    for(auto& s:juce::StringArray{"LP4","HP4","BP2","Notch"}) filterTypeBox.addItem(s,filterTypeBox.getNumItems()+1);
    styleCombo(filterTypeBox,green); addAndMakeVisible(filterTypeBox);
    filterTypeAttach=std::make_unique<CA>(apvts,"filterType",filterTypeBox);

    stutterModeBox.addItem("Free",1); stutterModeBox.addItem("Sync",2);
    styleCombo(stutterModeBox,pink); addAndMakeVisible(stutterModeBox);
    stutterModeAttach=std::make_unique<CA>(apvts,"stutterMode",stutterModeBox);
    for(auto& s:juce::StringArray{"1/4","1/8","1/16","1/32","1/64","1/128","1/4T","1/8T","1/16T","1/4Q","1/8Q","1/4S","1/8S"})
        stutterDivBox.addItem(s,stutterDivBox.getNumItems()+1);
    styleCombo(stutterDivBox,pink); addAndMakeVisible(stutterDivBox);
    stutterDivAttach=std::make_unique<CA>(apvts,"stutterDiv",stutterDivBox);

    for(auto& s:juce::StringArray{">> Pitch",">> Cutoff",">> PWM",">> Morph"})
        lfo2TargetBox.addItem(s,lfo2TargetBox.getNumItems()+1);
    styleCombo(lfo2TargetBox,blue); addAndMakeVisible(lfo2TargetBox);
    lfo2TargetAttach=std::make_unique<CA>(apvts,"lfo2Target",lfo2TargetBox);

    for(auto& s:juce::StringArray{"Up","Down","UpDown","Random"}) arpModeBox.addItem(s,arpModeBox.getNumItems()+1);
    for(auto& s:juce::StringArray{"1/4","1/8","1/16","1/32"})     arpDivBox.addItem(s,arpDivBox.getNumItems()+1);
    for(auto& s:juce::StringArray{"1","2","3","4"})               arpOctBox.addItem(s,arpOctBox.getNumItems()+1);
    styleCombo(arpModeBox,cyan); styleCombo(arpDivBox,cyan); styleCombo(arpOctBox,cyan);
    addAndMakeVisible(arpModeBox); addAndMakeVisible(arpDivBox); addAndMakeVisible(arpOctBox);
    arpModeAttach=std::make_unique<CA>(apvts,"arpMode",arpModeBox);
    arpDivAttach =std::make_unique<CA>(apvts,"arpDiv", arpDivBox);
    arpOctAttach =std::make_unique<CA>(apvts,"arpOctaves",arpOctBox);

    for(int i=0;i<128;++i){
        smpRootBox.addItem(juce::MidiMessage::getMidiNoteName(i,true,true,4),i+1);
        smp2RootBox.addItem(juce::MidiMessage::getMidiNoteName(i,true,true,4),i+1);
    }
    smpLoopBox.addItem("Off",1); smpLoopBox.addItem("Forward",2); smpLoopBox.addItem("PingPong",3);
    styleCombo(smpRootBox,smpC);  addAndMakeVisible(smpRootBox);  smpRootAttach=std::make_unique<CA>(apvts,"smpRoot",smpRootBox);
    styleCombo(smpLoopBox,smpC);  addAndMakeVisible(smpLoopBox);  smpLoopAttach=std::make_unique<CA>(apvts,"smpLoop",smpLoopBox);
    styleCombo(smp2RootBox,smp2C);addAndMakeVisible(smp2RootBox); smp2RootAttach=std::make_unique<CA>(apvts,"smp2Root",smp2RootBox);

    // Sample file labels + load buttons
    auto setupSmpLabel=[this](juce::Label& l,juce::Colour c){
        l.setText("-- no sample --",juce::dontSendNotification);
        l.setFont(juce::FontOptions("Helvetica Neue",8.f,juce::Font::plain));
        l.setColour(juce::Label::textColourId,c.withAlpha(0.7f));
        addAndMakeVisible(l);
    };
    setupSmpLabel(smpFileLabel, smpC); setupSmpLabel(smp2FileLabel,smp2C);

    auto styleLoadBtn=[&](juce::TextButton& b,juce::Colour c){
        b.setColour(juce::TextButton::buttonColourId,juce::Colour(0xFF181818));
        b.setColour(juce::TextButton::textColourOffId,c);
        addAndMakeVisible(b);
    };
    styleLoadBtn(smpLoadBtn, smpC);
    styleLoadBtn(smp2LoadBtn,smp2C);

    smpLoadBtn.onClick=[this]{
        fileChooser=std::make_unique<juce::FileChooser>("Load Sample",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.wav;*.aiff;*.aif;*.flac;*.mp3");
        fileChooser->launchAsync(
            juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc){
                auto f=fc.getResult(); if(!f.existsAsFile()) return;
                processor.getSharedSample().loadFile(f);
                smpFileLabel.setText(f.getFileName(),juce::dontSendNotification);
            });
    };
    smp2LoadBtn.onClick=[this]{
        fileChooser2=std::make_unique<juce::FileChooser>("Load Sample 2",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.wav;*.aiff;*.aif;*.flac;*.mp3");
        fileChooser2->launchAsync(
            juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc){
                auto f=fc.getResult(); if(!f.existsAsFile()) return;
                processor.getSharedSample2().loadFile(f);
                smp2FileLabel.setText(f.getFileName(),juce::dontSendNotification);
            });
    };

    // ── Knobs ─────────────────────────────────────────────────────
    styleKnob(pwmKnob,     pwmLabel,     "PWM",      yellow);
    styleKnob(osc1VolKnob, osc1VolLabel, "VOL",      green);
    styleKnob(gran1Knob,   gran1Label,   "GRAN 1",   teal);
    styleKnob(gran2Knob,   gran2Label,   "GRAN 2",   teal);
    styleKnob(osc1FCKnob,  osc1FCLabel,  "CUT",      orange);

    styleKnob(osc2VolKnob, osc2VolLabel, "VOL",      lime);
    styleKnob(osc2DetKnob, osc2DetLabel, "DETUNE",   lime);
    styleKnob(osc2PwmKnob, osc2PwmLabel, "PWM",      lime);
    styleKnob(osc2FCKnob,  osc2FCLabel,  "CUT",      orange);

    styleKnob(cutoffKnob,    cutoffLabel,    "CUTOFF",   green);
    styleKnob(resonanceKnob, resonanceLabel, "RESO",     green);
    styleKnob(filterEnvKnob, filterEnvLabel, "ENV >",    green);
    styleKnob(driveKnob,     driveLabel,     "DRIVE",    orange);
    styleKnob(lfoRateKnob,   lfoRateLabel,   "LFO RATE", orange);
    styleKnob(lfoDepthKnob,  lfoDepthLabel,  "LFO DEPT", orange);

    styleKnob(attackKnob,  attackLabel,  "ATK",   cyan);
    styleKnob(decayKnob,   decayLabel,   "DEC",   cyan);
    styleKnob(sustainKnob, sustainLabel, "SUS",   cyan);
    styleKnob(releaseKnob, releaseLabel, "REL",   cyan);
    styleKnob(portaKnob,   portaLabel,   "PORTA", yellow);

    styleKnob(chaosKnob,     chaosLabel,     "SMEAR",   pink);
    styleKnob(chaos2Knob,    chaos2Label,    "FRAKTAL",  pink);
    styleKnob(stutterKnob,   stutterLabel,   "STUTTER",  pink);
    styleKnob(morphKnob,     morphLabel,     "MORPH",    violet);
    styleKnob(morphAutoKnob, morphAutoLabel, "AUTO",     violet);
    styleKnob(phaseDestKnob, phaseDestLabel, "PHASE",    violet);
    styleKnob(timeFoldKnob,  timeFoldLabel,  "TIME",     violet);

    styleKnob(osc3VolKnob,  osc3VolLabel,  "VOL",   smpC);
    styleKnob(smpStartKnob, smpStartLabel, "START", smpC);
    styleKnob(smpEndKnob,   smpEndLabel,   "END",   smpC);
    styleKnob(smp1FCKnob,   smp1FCLabel,   "CUT",   orange);

    styleKnob(osc4VolKnob,   osc4VolLabel,   "VOL",   smp2C);
    styleKnob(smp2StartKnob, smp2StartLabel, "START", smp2C);
    styleKnob(smp2EndKnob,   smp2EndLabel,   "END",   smp2C);
    styleKnob(smp2GrainKnob, smp2GrainLabel, "GRAIN", smp2C);
    styleKnob(smp2AccKnob,   smp2AccLabel,   "ACCEL", smp2C);
    styleKnob(smp2FCKnob,    smp2FCLabel,    "CUT",   orange);

    styleKnob(lfo2AttackKnob, lfo2AttackLabel, "ATK",   blue);
    styleKnob(lfo2DecayKnob,  lfo2DecayLabel,  "DEC",   blue);
    styleKnob(lfo2RateKnob,   lfo2RateLabel,   "RATE",  blue);
    styleKnob(lfo2DepthKnob,  lfo2DepthLabel,  "DEPTH", blue);
    styleKnob(lfo2SustainKnob,lfo2SustainLabel,"SUS",   blue);
    styleKnob(lfo2ReleaseKnob,lfo2ReleaseLabel,"REL",   blue);

    styleKnob(delayMixKnob,  delayMixLabel,  "DLY MIX",  red);
    styleKnob(delayTimeKnob, delayTimeLabel, "DLY TIME", red);
    styleKnob(delayFBKnob,   delayFBLabel,   "DLY FB",   red);
    styleKnob(reverbMixKnob, reverbMixLabel, "REV MIX",  violet);
    styleKnob(reverbSizeKnob,reverbSizeLabel,"REV SIZE", violet);
    styleKnob(uniDetuneKnob, uniDetuneLabel, "UNI DET",  orange);
    styleKnob(uniSpreadKnob, uniSpreadLabel, "UNI SPR",  orange);
    styleKnob(volumeKnob,    volumeLabel,    "MASTER",   green);

    // ── Slider Attachments ────────────────────────────────────────
    pwmAttach        =std::make_unique<SA>(apvts,"pwm",          pwmKnob);
    osc1VolAttach    =std::make_unique<SA>(apvts,"osc1Vol",      osc1VolKnob);
    gran1Attach      =std::make_unique<SA>(apvts,"gran1",        gran1Knob);
    gran2Attach      =std::make_unique<SA>(apvts,"gran2",        gran2Knob);
    osc1FCAttach     =std::make_unique<SA>(apvts,"osc1FilterCutoff",osc1FCKnob);
    osc2VolAttach    =std::make_unique<SA>(apvts,"osc2Vol",      osc2VolKnob);
    osc2DetAttach    =std::make_unique<SA>(apvts,"osc2Detune",   osc2DetKnob);
    osc2PwmAttach    =std::make_unique<SA>(apvts,"osc2Pwm",      osc2PwmKnob);
    osc2FCAttach     =std::make_unique<SA>(apvts,"osc2FilterCutoff",osc2FCKnob);
    cutoffAttach     =std::make_unique<SA>(apvts,"cutoff",       cutoffKnob);
    resonanceAttach  =std::make_unique<SA>(apvts,"resonance",    resonanceKnob);
    filterEnvAttach  =std::make_unique<SA>(apvts,"filterEnvAmt", filterEnvKnob);
    driveAttach      =std::make_unique<SA>(apvts,"drive",        driveKnob);
    lfoRateAttach    =std::make_unique<SA>(apvts,"lfoRate",      lfoRateKnob);
    lfoDepthAttach   =std::make_unique<SA>(apvts,"lfoDepth",     lfoDepthKnob);
    attackAttach     =std::make_unique<SA>(apvts,"attack",       attackKnob);
    decayAttach      =std::make_unique<SA>(apvts,"decay",        decayKnob);
    sustainAttach    =std::make_unique<SA>(apvts,"sustain",      sustainKnob);
    releaseAttach    =std::make_unique<SA>(apvts,"release",      releaseKnob);
    portaAttach      =std::make_unique<SA>(apvts,"portamento",   portaKnob);
    chaosAttach      =std::make_unique<SA>(apvts,"chaos",        chaosKnob);
    chaos2Attach     =std::make_unique<SA>(apvts,"chaos2",       chaos2Knob);
    stutterAttach    =std::make_unique<SA>(apvts,"stutter",      stutterKnob);
    morphAttach      =std::make_unique<SA>(apvts,"morph",        morphKnob);
    morphAutoAttach  =std::make_unique<SA>(apvts,"morphAuto",    morphAutoKnob);
    phaseDestAttach  =std::make_unique<SA>(apvts,"phaseDest",    phaseDestKnob);
    timeFoldAttach   =std::make_unique<SA>(apvts,"timeFold",     timeFoldKnob);
    osc3VolAttach    =std::make_unique<SA>(apvts,"osc3Vol",      osc3VolKnob);
    smpStartAttach   =std::make_unique<SA>(apvts,"smpStart",     smpStartKnob);
    smpEndAttach     =std::make_unique<SA>(apvts,"smpEnd",       smpEndKnob);
    smp1FCAttach     =std::make_unique<SA>(apvts,"smp1FilterCutoff",smp1FCKnob);
    osc4VolAttach    =std::make_unique<SA>(apvts,"osc4Vol",      osc4VolKnob);
    smp2StartAttach  =std::make_unique<SA>(apvts,"smp2Start",    smp2StartKnob);
    smp2EndAttach    =std::make_unique<SA>(apvts,"smp2End",      smp2EndKnob);
    smp2GrainAttach  =std::make_unique<SA>(apvts,"smp2GrainRate",smp2GrainKnob);
    smp2AccAttach    =std::make_unique<SA>(apvts,"smp2StutterAcc",smp2AccKnob);
    smp2FCAttach     =std::make_unique<SA>(apvts,"smp2FilterCutoff",smp2FCKnob);
    lfo2AttackAttach =std::make_unique<SA>(apvts,"lfo2Attack",   lfo2AttackKnob);
    lfo2DecayAttach  =std::make_unique<SA>(apvts,"lfo2Decay",    lfo2DecayKnob);
    lfo2RateAttach   =std::make_unique<SA>(apvts,"lfo2Rate",     lfo2RateKnob);
    lfo2DepthAttach  =std::make_unique<SA>(apvts,"lfo2Depth",    lfo2DepthKnob);
    lfo2SustainAttach=std::make_unique<SA>(apvts,"lfo2Sustain",  lfo2SustainKnob);
    lfo2ReleaseAttach=std::make_unique<SA>(apvts,"lfo2Release",  lfo2ReleaseKnob);
    delayMixAttach   =std::make_unique<SA>(apvts,"delayMix",     delayMixKnob);
    delayTimeAttach  =std::make_unique<SA>(apvts,"delayTime",    delayTimeKnob);
    delayFBAttach    =std::make_unique<SA>(apvts,"delayFeedback",delayFBKnob);
    reverbMixAttach  =std::make_unique<SA>(apvts,"reverbMix",    reverbMixKnob);
    reverbSizeAttach =std::make_unique<SA>(apvts,"reverbSize",   reverbSizeKnob);
    uniDetuneAttach  =std::make_unique<SA>(apvts,"unisonDetune", uniDetuneKnob);
    uniSpreadAttach  =std::make_unique<SA>(apvts,"unisonSpread", uniSpreadKnob);
    volumeAttach     =std::make_unique<SA>(apvts,"volume",       volumeKnob);
}

Fraktal611AudioProcessorEditor::~Fraktal611AudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void Fraktal611AudioProcessorEditor::timerCallback()
{
    updateKeyboardColour();
}

void Fraktal611AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(C_BG);
    g.setColour(C_ACCENT); g.fillRect(0,0,W,2);
    g.setFont(juce::FontOptions("Helvetica Neue",11.f,juce::Font::bold));
    g.setColour(C_ACCENT); g.drawText("FRAKTAL 611",8,5,160,14,juce::Justification::left);
    g.setFont(juce::FontOptions("Helvetica Neue",7.f,juce::Font::plain));
    g.setColour(C_MUTED);
    g.drawText("dual osc · grain stutter · per-osc filter · section bypass · sample · morph · chaos · lfo2 · arp",
               172,5,700,14,juce::Justification::left);

    g.setColour(juce::Colour(0xFF0D0D0D)); g.fillRect(0,18,W,PBAR-18);
    g.setColour(C_DIM); g.fillRect(0,PBAR,W,1);
    // Stärkere Trennlinie zwischen Header und Sektionen
    g.setColour(C_ACCENT.withAlpha(0.15f)); g.fillRect(0,PBAR,W,1);
    g.setFont(juce::FontOptions("Helvetica Neue",7.f,juce::Font::plain));
    g.setColour(C_MUTED); g.drawText("PRESET",8,22,46,12,juce::Justification::left);

    if(processor.isMidiLearnActive()){
        g.setColour(juce::Colour(0xFFFF4488));
        g.drawText("● MIDI LEARN",W-110,22,102,12,juce::Justification::right);
    }

    const juce::Colour lime(0xFF88FF44),smpC(0xFFFF9944),smp2C(0xFFFFCC44);
    const juce::Colour blue(0xFF44AAFF),red(0xFFFF5555),violet(0xFFCC44FF),pink(0xFFFF4488);

    // Divider
    g.setColour(C_DIM);
    for(int i=1;i<5;++i){
        g.fillRect(i*SEC,PBAR+1,1,RH+SHDR+2);
        g.fillRect(i*SEC,R2Y-SHDR,1,RH+SHDR+2);
    }
    g.fillRect(0,R2Y-SHDR-2,W,1);

    // Section headers Reihe 1
    auto hdr=[&](const juce::String& t,int x,juce::Colour c){
        g.setFont(juce::FontOptions("Helvetica Neue",7.5f,juce::Font::plain));
        g.setColour(c); g.drawText(t,juce::Rectangle<int>{x+2,PBAR+2,SEC-4,SHDR-2},juce::Justification::left);
    };
    hdr("OSC 1",  SX0, C_MUTED);
    hdr("OSC 2",  SX1, lime);
    hdr("FILTER", SX2, C_MUTED);
    hdr("ADSR",   SX3, C_MUTED);
    hdr("FX",     SX4, pink);

    // Section headers Reihe 2
    int r2h=R2Y-SHDR;
    auto hdr2=[&](const juce::String& t,int x,juce::Colour c){
        g.setFont(juce::FontOptions("Helvetica Neue",7.5f,juce::Font::plain));
        g.setColour(c); g.drawText(t,juce::Rectangle<int>{x+2,r2h+2,SEC-4,SHDR-2},juce::Justification::left);
    };
    hdr2("SMP 1",          SX0, smpC);
    hdr2("SMP 2  GRAIN",   SX1, smp2C);
    hdr2("LFO 2",          SX2, blue);
    hdr2("ARPEGGIATOR",    SX3, juce::Colour(0xFF00DDCC));
    hdr2("MASTER",         SX4, C_MUTED);

    // Glows
    g.setColour(lime.withAlpha(0.03f));   g.fillRect(SX1,PBAR,SEC,RH+SHDR);
    g.setColour(smpC.withAlpha(0.03f));   g.fillRect(SX0,r2h,SEC,RH+SHDR);
    g.setColour(smp2C.withAlpha(0.03f));  g.fillRect(SX1,r2h,SEC,RH+SHDR);
    g.setColour(blue.withAlpha(0.03f));   g.fillRect(SX2,r2h,SEC,RH+SHDR);
    g.setColour(violet.withAlpha(0.02f)); g.fillRect(SX4,r2h,SEC,RH+SHDR);

    // Scope
    g.setFont(juce::FontOptions("Helvetica Neue",6.5f,juce::Font::plain));
    g.setColour(C_MUTED.withAlpha(0.4f));
    g.drawText("OSCILLOSCOPE  ·  right-click knob = MIDI learn",8,SCOPE_Y-10,W-16,9,juce::Justification::left);
    g.setColour(C_MUTED.withAlpha(0.4f));
    g.drawText("KEYBOARD  C2–C7",8,KEYS_Y-10,W-16,9,juce::Justification::left);

    // ══ ROW 3 Section Headers ════════════════════════════════════
    {
        struct R3S{int sx;const char* t;juce::Colour c;};
        R3S secs[]={
            {SX0,"LORENZ NAV",        juce::Colour(0xFFE87D3E)},
            {SX1,"GRAY-SCOTT RD",     juce::Colour(0xFF38C8A0)},
            {SX2,"WEIERSTRASS",       juce::Colour(0xFF7B68EE)},
            {SX3,"EVOL. DRIFT",       juce::Colour(0xFFFFCC00)},
            {SX4,"POLY-TEMPORAL LFO", juce::Colour(0xFF7B68EE)}
        };
        for(auto& s:secs){
            g.setColour(s.c.withAlpha(0.10f)); g.fillRect(s.sx,R3Y-SHDR,SEC,R3H+SHDR);
            g.setColour(juce::Colour(0xFF1A1A1A)); g.fillRect(s.sx,R3Y-SHDR,SEC,SHDR);
            g.setColour(juce::Colour(0xFF252525)); g.drawRect(s.sx,R3Y-SHDR,SEC,R3H+SHDR,1);
            g.setFont(juce::FontOptions("Helvetica Neue",8.f,juce::Font::bold));
            g.setColour(s.c);
            g.drawText(s.t,s.sx+PAD,R3Y-SHDR,SEC-PAD*2,SHDR,juce::Justification::centredLeft);
        }
        // Poly-LFO: Erklärung was die Multiplikatoren bedeuten
        g.setFont(juce::FontOptions(7.5f));
        g.setColour(juce::Colour(0xFF7B68EE).withAlpha(0.7f));
        g.drawText("Zwei LFOs niemals synchron:",SX4+PAD,K1C+76,SEC-PAD*2,13,juce::Justification::centredLeft);
        g.drawText(u8"φ=1.618  π=3.14  √2=1.41",SX4+PAD,K1C+90,SEC-PAD*2,13,juce::Justification::centredLeft);
        g.drawText(u8"√3=1.73  e=2.72",SX4+PAD,K1C+104,SEC-PAD*2,13,juce::Justification::centredLeft);
    }

}

void Fraktal611AudioProcessorEditor::resized()
{
    // Preset
    presetCategoryBox.setBounds(56,28,80,20);
    presetNameBox.setBounds(142,28,130,20);
    presetLoadBtn.setBounds(278,28,50,20);
    presetNameEdit.setBounds(334,28,110,20);
    presetSaveBtn.setBounds(450,28,50,20);
    randomBtn.setBounds(510,28,66,20);
    secFilterBtn.setBounds(W-320,28,68,20);
    secFXBtn.setBounds    (W-250,28,50,20);
    secLFO2Btn.setBounds  (W-198,28,60,20);
    monoBtn.setBounds     (W-136,28,64,20);
    panicBtn.setBounds    (W-70, 28,62,20);


    // ══════════════════════════════════════════════════════════════
    // ROW 3 Layout
    // ══════════════════════════════════════════════════════════════

    // LORENZ NAV (SX0)
    lorenzSigmaKnob.setBounds(kx(SX0,0),K1C,KW,KH); lorenzSigmaLabel.setBounds(kx(SX0,0),K1C+KH,KW,12);
    lorenzRhoKnob.setBounds  (kx(SX0,1),K1C,KW,KH); lorenzRhoLabel.setBounds  (kx(SX0,1),K1C+KH,KW,12);
    lorenzBetaKnob.setBounds (kx(SX0,2),K1C,KW,KH); lorenzBetaLabel.setBounds (kx(SX0,2),K1C+KH,KW,12);
    lorenzDtKnob.setBounds   (kx(SX0,0),K2C,KW,KH); lorenzDtLabel.setBounds   (kx(SX0,0),K2C+KH,KW,12);

    // GRAY-SCOTT (SX1)
    gsOnBtn.setBounds    (SX2-PAD-50,    R3Y-SHDR+2, 46, 18);  // rechts im Header
    gsFeedKnob.setBounds (kx(SX1,0),K1C,KW,KH); gsFeedLabel.setBounds(kx(SX1,0),K1C+KH,KW,11);
    gsKillKnob.setBounds (kx(SX1,1),K1C,KW,KH); gsKillLabel.setBounds(kx(SX1,1),K1C+KH,KW,12);
    gsDaKnob.setBounds   (kx(SX1,2),K1C,KW,KH); gsDaLabel.setBounds  (kx(SX1,2),K1C+KH,KW,12);
    gsDbKnob.setBounds   (kx(SX1,0),K2C,KW,KH); gsDbLabel.setBounds  (kx(SX1,0),K2C+KH,KW,12);
    gsVolKnob.setBounds  (kx(SX1,1),K2C,KW,KH); gsVolLabel.setBounds (kx(SX1,1),K2C+KH,KW,12);

    // WEIERSTRASS (SX2)
    weiDimKnob.setBounds (kx(SX2,0),K1C,KW,KH); weiDimLabel.setBounds(kx(SX2,0),K1C+KH,KW,12);
    weiLacBox.setBounds  (kx(SX2,1),K1C+8, 88, 22);
    weiDepthBox.setBounds(kx(SX2,1),K1C+34, 88, 22);

    // DRIFT (SX3)
    driftOnBtn.setBounds    (SX4-PAD-64,     R3Y-SHDR+2, 60, 18);  // rechts im Header
    driftSpeedKnob.setBounds(kx(SX3,0),K1C,KW,KH); driftSpeedLabel.setBounds(kx(SX3,0),K1C+KH,KW,12);
    driftRangeKnob.setBounds(kx(SX3,1),K1C,KW,KH); driftRangeLabel.setBounds(kx(SX3,1),K1C+KH,KW,12);
    driftModeBox.setBounds  (kx(SX3,0),DB3, 178, 22);

    // POLY-TEMPORAL LFO (SX4) — zwei Combo-Boxen klar beschriftet
    lfoMultLabel.setBounds (SX4+PAD,       K1C,     40, 16);
    lfoMultBox.setBounds   (SX4+PAD+44,    K1C,    140, 22);
    lfo2MultLabel.setBounds(SX4+PAD,       K1C+36,  40, 16);
    lfo2MultBox.setBounds  (SX4+PAD+44,    K1C+36, 140, 22);

    scope.setBounds(0, SCOPE_Y, W, SCOPE_H);
    // Keyboard: volle Breite, Key-Breite dynamisch berechnen (C2-C7 = 35 weiße Tasten)
    keyboard.setBounds(0, KEYS_Y, W, KEYS_H);
    keyboard.setKeyWidth((float)W / 35.f);

    // ── REIHE 1 ───────────────────────────────────────────────────

    // ── OSC 1 ─────────────────────────────────────────────────────
    // Header: Checkbox rechts (wie ARP ON), sauber
    osc1OnBtn.setBounds   (SX1-PAD-22,    PBAR+2,  22, 20);
    // K1: Wave-Dropdown volle Breite
    waveSelector.setBounds(SX0+PAD,       K1,     178, 22);
    // K1+26: Oct + LP + HP — klar getrennt von Wave
    osc1OctBox.setBounds  (SX0+PAD,       K1+26,   50, 20);
    osc1FOnBtn.setBounds  (SX0+PAD+54,    K1+26,   58, 20);
    osc1FHPBtn.setBounds  (SX0+PAD+116,   K1+26,   54, 20);
    // K2: PWM + VOL + CUT
    pwmKnob.setBounds     (kx(SX0,0),K2,KW,KH); pwmLabel.setBounds    (kx(SX0,0),K2+KH,KW,12);
    osc1VolKnob.setBounds (kx(SX0,1),K2,KW,KH); osc1VolLabel.setBounds(kx(SX0,1),K2+KH,KW,12);
    osc1FCKnob.setBounds  (kx(SX0,2),K2,KW,KH); osc1FCLabel.setBounds (kx(SX0,2),K2+KH,KW,12);
    // K3: GRAN1 + GRAN2
    gran1Knob.setBounds   (kx(SX0,0),K3,KW,KH); gran1Label.setBounds  (kx(SX0,0),K3+KH,KW,12);
    gran2Knob.setBounds   (kx(SX0,1),K3,KW,KH); gran2Label.setBounds  (kx(SX0,1),K3+KH,KW,12);

    // ── OSC 2 ─────────────────────────────────────────────────────
    osc2OnBtn.setBounds   (SX2-PAD-22,    PBAR+2,  22, 20);
    wave2Selector.setBounds(SX1+PAD,      K1,     178, 22);
    osc2OctBox.setBounds  (SX1+PAD,       K1+26,   50, 20);
    osc2FOnBtn.setBounds  (SX1+PAD+54,    K1+26,   58, 20);
    osc2FHPBtn.setBounds  (SX1+PAD+116,   K1+26,   54, 20);
    osc2VolKnob.setBounds (kx(SX1,0),K2,KW,KH); osc2VolLabel.setBounds(kx(SX1,0),K2+KH,KW,12);
    osc2DetKnob.setBounds (kx(SX1,1),K2,KW,KH); osc2DetLabel.setBounds(kx(SX1,1),K2+KH,KW,12);
    osc2PwmKnob.setBounds (kx(SX1,2),K2,KW,KH); osc2PwmLabel.setBounds(kx(SX1,2),K2+KH,KW,12);
    osc2FCKnob.setBounds  (kx(SX1,0),K3,KW,KH); osc2FCLabel.setBounds (kx(SX1,0),K3+KH,KW,12);

    // FILTER
    cutoffKnob.setBounds    (kx(SX2,0),K1,KW,KH); cutoffLabel.setBounds    (kx(SX2,0),K1+KH,KW,12);
    resonanceKnob.setBounds (kx(SX2,1),K1,KW,KH); resonanceLabel.setBounds (kx(SX2,1),K1+KH,KW,12);
    filterEnvKnob.setBounds (kx(SX2,2),K1,KW,KH); filterEnvLabel.setBounds (kx(SX2,2),K1+KH,KW,12);
    driveKnob.setBounds     (kx(SX2,0),K2,KW,KH); driveLabel.setBounds     (kx(SX2,0),K2+KH,KW,12);
    lfoRateKnob.setBounds   (kx(SX2,1),K2,KW,KH); lfoRateLabel.setBounds   (kx(SX2,1),K2+KH,KW,12);
    lfoDepthKnob.setBounds  (kx(SX2,2),K2,KW,KH); lfoDepthLabel.setBounds  (kx(SX2,2),K2+KH,KW,12);
    filterTypeBox.setBounds (kx(SX2,0),DB1,178,22);

    // ADSR
    attackKnob.setBounds  (kx(SX3,0),K1,KW,KH); attackLabel.setBounds  (kx(SX3,0),K1+KH,KW,12);
    decayKnob.setBounds   (kx(SX3,1),K1,KW,KH); decayLabel.setBounds   (kx(SX3,1),K1+KH,KW,12);
    sustainKnob.setBounds (kx(SX3,0),K2,KW,KH); sustainLabel.setBounds (kx(SX3,0),K2+KH,KW,12);
    releaseKnob.setBounds (kx(SX3,1),K2,KW,KH); releaseLabel.setBounds (kx(SX3,1),K2+KH,KW,12);
    portaKnob.setBounds   (kx(SX3,0),K3,KW,KH); portaLabel.setBounds   (kx(SX3,0),K3+KH,KW,12);

    // FX
    chaosKnob.setBounds    (kx(SX4,0),K1,KW,KH); chaosLabel.setBounds    (kx(SX4,0),K1+KH,KW,12);
    chaos2Knob.setBounds   (kx(SX4,1),K1,KW,KH); chaos2Label.setBounds   (kx(SX4,1),K1+KH,KW,12);
    stutterKnob.setBounds  (kx(SX4,2),K1,KW,KH); stutterLabel.setBounds  (kx(SX4,2),K1+KH,KW,12);
    morphKnob.setBounds    (kx(SX4,0),K2,KW,KH); morphLabel.setBounds    (kx(SX4,0),K2+KH,KW,12);
    morphAutoKnob.setBounds(kx(SX4,1),K2,KW,KH); morphAutoLabel.setBounds(kx(SX4,1),K2+KH,KW,12);
    phaseDestKnob.setBounds(kx(SX4,2),K2,KW,KH); phaseDestLabel.setBounds(kx(SX4,2),K2+KH,KW,12);
    timeFoldKnob.setBounds (kx(SX4,0),K3,KW,KH); timeFoldLabel.setBounds (kx(SX4,0),K3+KH,KW,12);
    stutterModeBox.setBounds(kx(SX4,1),DB1-2,82,22);
    stutterDivBox.setBounds (kx(SX4,1)+86,DB1-2,80,22);

    // ── REIHE 2 ───────────────────────────────────────────────────

    // ── SMP 1 ─────────────────────────────────────────────────────
    // Header: Checkbox rechts
    osc3OnBtn.setBounds   (SX1-PAD-22,     R2Y-SHDR+2, 22, 18);
    // K1B: LOAD SAMPLE Button volle Breite
    smpLoadBtn.setBounds  (SX0+PAD,        K1B,       178, 22);
    smpFileLabel.setBounds(SX0+PAD,        K1B+26,    178, 14);
    // K2B: VOL + START + END
    osc3VolKnob.setBounds (kx(SX0,0),K2B,KW,KH); osc3VolLabel.setBounds (kx(SX0,0),K2B+KH,KW,12);
    smpStartKnob.setBounds(kx(SX0,1),K2B,KW,KH); smpStartLabel.setBounds(kx(SX0,1),K2B+KH,KW,12);
    smpEndKnob.setBounds  (kx(SX0,2),K2B,KW,KH); smpEndLabel.setBounds  (kx(SX0,2),K2B+KH,KW,12);
    // K3B: FILT toggle + CUT knob
    smp1FOnBtn.setBounds  (SX0+PAD,        K3B+14, 52, 20);
    smp1FCKnob.setBounds  (SX0+PAD+58,     K3B,    KW, KH);
    smp1FCLabel.setBounds (SX0+PAD+58,     K3B+KH, KW, 12);
    // DB2: Root + Loop
    smpRootBox.setBounds  (SX0+PAD,        DB2, 82, 22);
    smpLoopBox.setBounds  (SX0+PAD+86,     DB2, 90, 22);

    // ── SMP 2 (Grain Stutter) ─────────────────────────────────────
    osc4OnBtn.setBounds   (SX2-PAD-22,     R2Y-SHDR+2, 22, 18);
    smp2LoadBtn.setBounds (SX1+PAD,        K1B,       178, 22);
    smp2FileLabel.setBounds(SX1+PAD,       K1B+26,    178, 14);
    // K2B: VOL + START + END
    osc4VolKnob.setBounds  (kx(SX1,0),K2B,KW,KH); osc4VolLabel.setBounds  (kx(SX1,0),K2B+KH,KW,12);
    smp2StartKnob.setBounds(kx(SX1,1),K2B,KW,KH); smp2StartLabel.setBounds(kx(SX1,1),K2B+KH,KW,12);
    smp2EndKnob.setBounds  (kx(SX1,2),K2B,KW,KH); smp2EndLabel.setBounds  (kx(SX1,2),K2B+KH,KW,12);
    // K3B: GRAIN + ACCEL + CUT
    smp2GrainKnob.setBounds(kx(SX1,0),K3B,KW,KH); smp2GrainLabel.setBounds(kx(SX1,0),K3B+KH,KW,12);
    smp2AccKnob.setBounds  (kx(SX1,1),K3B,KW,KH); smp2AccLabel.setBounds  (kx(SX1,1),K3B+KH,KW,12);
    smp2FCKnob.setBounds   (kx(SX1,2),K3B,KW,KH); smp2FCLabel.setBounds   (kx(SX1,2),K3B+KH,KW,12);
    // DB2: FILT + Root
    smp2FOnBtn.setBounds  (SX1+PAD,        DB2, 52, 22);
    smp2RootBox.setBounds (SX1+PAD+56,     DB2, 96, 22);

    // LFO 2
    lfo2AttackKnob.setBounds (kx(SX2,0),K1B,KW,KH); lfo2AttackLabel.setBounds (kx(SX2,0),K1B+KH,KW,12);
    lfo2DecayKnob.setBounds  (kx(SX2,1),K1B,KW,KH); lfo2DecayLabel.setBounds  (kx(SX2,1),K1B+KH,KW,12);
    lfo2RateKnob.setBounds   (kx(SX2,0),K2B,KW,KH); lfo2RateLabel.setBounds   (kx(SX2,0),K2B+KH,KW,12);
    lfo2DepthKnob.setBounds  (kx(SX2,1),K2B,KW,KH); lfo2DepthLabel.setBounds  (kx(SX2,1),K2B+KH,KW,12);
    lfo2SustainKnob.setBounds(kx(SX2,0),K3B,KW,KH); lfo2SustainLabel.setBounds(kx(SX2,0),K3B+KH,KW,12);
    lfo2ReleaseKnob.setBounds(kx(SX2,1),K3B,KW,KH); lfo2ReleaseLabel.setBounds(kx(SX2,1),K3B+KH,KW,12);
    lfo2TargetBox.setBounds  (kx(SX2,0),DB2,178,22);

    // ARP — große sichtbare Checkbox im Header, Dropdowns tief unten
    arpOnBtn.setBounds  (SX3+PAD+100,  R2Y-SHDR+2, 78, 18);   // rechts im Header, breit+sichtbar
    arpModeBox.setBounds(kx(SX3,0),    K2B,        178, 22);   // erst ab K2B
    arpDivBox.setBounds (kx(SX3,0),    K3B,        116, 22);
    arpOctBox.setBounds (kx(SX3,0)+120,K3B,         56, 22);

    // MASTER
    delayMixKnob.setBounds   (kx(SX4,0),K1B,KW,KH); delayMixLabel.setBounds   (kx(SX4,0),K1B+KH,KW,12);
    delayTimeKnob.setBounds  (kx(SX4,1),K1B,KW,KH); delayTimeLabel.setBounds  (kx(SX4,1),K1B+KH,KW,12);
    delayFBKnob.setBounds    (kx(SX4,2),K1B,KW,KH); delayFBLabel.setBounds    (kx(SX4,2),K1B+KH,KW,12);
    reverbMixKnob.setBounds  (kx(SX4,0),K2B,KW,KH); reverbMixLabel.setBounds  (kx(SX4,0),K2B+KH,KW,12);
    reverbSizeKnob.setBounds (kx(SX4,1),K2B,KW,KH); reverbSizeLabel.setBounds (kx(SX4,1),K2B+KH,KW,12);
    volumeKnob.setBounds     (kx(SX4,2),K2B,KW,KH); volumeLabel.setBounds     (kx(SX4,2),K2B+KH,KW,12);
    uniDetuneKnob.setBounds  (kx(SX4,0),K3B,KW,KH); uniDetuneLabel.setBounds  (kx(SX4,0),K3B+KH,KW,12);
    uniSpreadKnob.setBounds  (kx(SX4,1),K3B,KW,KH); uniSpreadLabel.setBounds  (kx(SX4,1),K3B+KH,KW,12);
}

void Fraktal611AudioProcessorEditor::styleKnob(
    juce::Slider& s,juce::Label& l,const juce::String& name,juce::Colour ac)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
    s.setColour(juce::Slider::rotarySliderFillColourId,   ac);
    s.setColour(juce::Slider::rotarySliderOutlineColourId,juce::Colour(0xFF1E1E1E));
    s.setColour(juce::Slider::thumbColourId,              ac);
    s.setColour(juce::Slider::backgroundColourId,         juce::Colour(0xFF141414));
    addAndMakeVisible(s);
    l.setText(name,juce::dontSendNotification);
    l.setFont(juce::FontOptions("Helvetica Neue",7.f,juce::Font::plain));
    l.setColour(juce::Label::textColourId,ac.withAlpha(0.75f));
    l.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(l);
}

void Fraktal611AudioProcessorEditor::styleCombo(juce::ComboBox& b,juce::Colour c)
{
    b.setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xFF141414));
    b.setColour(juce::ComboBox::textColourId,      c);
    b.setColour(juce::ComboBox::outlineColourId,   juce::Colour(0xFF2A2A2A));
    b.setColour(juce::ComboBox::arrowColourId,     c);
}

void Fraktal611AudioProcessorEditor::styleToggle(juce::ToggleButton& b,juce::Colour c)
{
    b.setColour(juce::ToggleButton::textColourId,        c);
    b.setColour(juce::ToggleButton::tickColourId,        c);
    b.setColour(juce::ToggleButton::tickDisabledColourId,juce::Colour(0xFF333333));
    addAndMakeVisible(b);
}

void Fraktal611AudioProcessorEditor::drawSection(
    juce::Graphics& g,const juce::String& t,juce::Rectangle<int> b,juce::Colour c,bool on)
{
    g.setFont(juce::FontOptions("Helvetica Neue",7.5f,juce::Font::plain));
    g.setColour(on?c:c.withAlpha(0.35f));
    g.drawText(t,b,juce::Justification::centred);
}

void Fraktal611AudioProcessorEditor::refreshPresetList()
{
    auto cat=PresetManager::getCategories()[presetCategoryBox.getSelectedItemIndex()];
    auto ps=PresetManager::getPresetsInCategory(cat);
    presetNameBox.clear();
    for(int i=0;i<ps.size();++i) presetNameBox.addItem(ps[i],i+1);
    if(ps.size()>0) presetNameBox.setSelectedItemIndex(0);
}

void Fraktal611AudioProcessorEditor::randomiseAll()
{
    auto& apvts=processor.apvts;
    auto& rng=juce::Random::getSystemRandom();
    auto rF=[&](const juce::String& id,float mn,float mx){
        if(auto* p=dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(id)))
            p->setValueNotifyingHost(p->convertTo0to1(mn+rng.nextFloat()*(mx-mn)));
    };
    auto rN=[&](const juce::String& id){
        if(auto* p=apvts.getParameter(id)) p->setValueNotifyingHost(rng.nextFloat());
    };
    auto rC=[&](const juce::String& id,int n){
        if(auto* p=apvts.getParameter(id))
            p->setValueNotifyingHost((float)rng.nextInt(n)/(float)juce::jmax(1,n-1));
    };
    rC("wave",6); rC("osc1Oct",5); rC("wave2",6); rC("osc2Oct",5); rC("filterType",4);
    rN("pwm"); rN("osc1Vol"); rF("osc2Vol",0.f,0.7f); rF("osc2Detune",-30.f,30.f); rN("osc2Pwm");
    rF("cutoff",200.f,7000.f); rN("resonance"); rN("drive"); rN("filterEnvAmt");
    rF("lfoRate",0.1f,8.f); rN("lfoDepth");
    rF("lfo2Rate",0.1f,12.f); rN("lfo2Depth"); rC("lfo2Target",4);
    rF("lfo2Attack",0.001f,2.f); rF("lfo2Decay",0.01f,2.f); rN("lfo2Sustain"); rF("lfo2Release",0.01f,2.f);
    rF("attack",0.001f,1.f); rF("decay",0.05f,1.5f); rN("sustain"); rF("release",0.05f,2.f);
    rF("gran1",0.f,0.4f); rF("gran2",0.f,0.4f);
    rF("chaos",0.f,0.6f); rN("chaos2"); rF("stutter",0.f,0.5f);
    rF("morph",0.f,5.f); rN("morphAuto"); rN("phaseDest"); rN("timeFold");
    rF("portamento",0.f,0.4f); rF("unisonDetune",0.f,20.f); rN("unisonSpread");
    rF("delayMix",0.f,0.5f); rF("delayTime",0.1f,1.f); rF("delayFeedback",0.1f,0.6f);
    rF("reverbMix",0.f,0.5f); rN("reverbSize"); rF("volume",0.5f,1.f);
}

void Fraktal611AudioProcessorEditor::updateKeyboardColour()
{
    // Farbe basierend auf dominantem Effekt/OSC
    auto& apvts = processor.apvts;
    float chaos2  = *apvts.getRawParameterValue("chaos2");
    float chaos   = *apvts.getRawParameterValue("chaos");
    float osc3On  = *apvts.getRawParameterValue("osc3On");
    float osc4On  = *apvts.getRawParameterValue("osc4On");
    float morph   = *apvts.getRawParameterValue("morph") / 5.f;
    float stutter = *apvts.getRawParameterValue("stutter");

    juce::Colour keyColour;
    if      (chaos2  > 0.5f) keyColour = juce::Colour(0xFFFF4488); // Fraktal: pink
    else if (stutter > 0.4f) keyColour = juce::Colour(0xFFFF8C00); // Stutter: orange
    else if (osc4On  > 0.5f) keyColour = juce::Colour(0xFFFFCC44); // Grain: gelb
    else if (osc3On  > 0.5f) keyColour = juce::Colour(0xFFFF9944); // Sample: smpC
    else if (chaos   > 0.5f) keyColour = juce::Colour(0xFF00DDCC); // Smear: cyan
    else if (morph   > 0.6f) keyColour = juce::Colour(0xFFCC44FF); // Morph: violet
    else                     keyColour = juce::Colour(0xFFC8FF00); // Default: green

    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,
                       keyColour.withAlpha(0.85f));
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                       keyColour.withAlpha(0.3f));
}

juce::AudioProcessorEditor* Fraktal611AudioProcessor::createEditor()
{
    return new Fraktal611AudioProcessorEditor(*this);
}
