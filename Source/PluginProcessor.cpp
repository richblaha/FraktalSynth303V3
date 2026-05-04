#include "PluginProcessor.h"
#include "PluginEditor.h"

// sharedSample2 als file-level static — kein Member in der Klasse nötig
static SharedSampleBuffer s_sharedSample2;

juce::AudioProcessorValueTreeState::ParameterLayout
Fraktal611AudioProcessor::createParameters()
{
    using F  = juce::AudioParameterFloat;
    using C  = juce::AudioParameterChoice;
    using B  = juce::AudioParameterBool;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ── OSC 1 ─────────────────────────────────────────────────────
    layout.add (std::make_unique<C> ("wave", "OSC1 Wave",
        juce::StringArray { "Lorenz","FM","Granular","Saw","Square","Drift" }, 0));
    layout.add (std::make_unique<F> ("pwm",    "PWM",    0.05f, 0.95f, 0.5f));
    layout.add (std::make_unique<F> ("osc1Vol","OSC1 Vol",0.0f, 1.0f,  0.8f));
    layout.add (std::make_unique<C> ("osc1Oct", "OSC1 Octave",
        juce::StringArray { "-2","-1","0","+1","+2" }, 2));
    layout.add (std::make_unique<B> ("osc1On", "OSC1 On", true));
    // OSC1 Mini-Filter
    layout.add (std::make_unique<B> ("osc1FilterOn", "OSC1 Filter On", false));
    layout.add (std::make_unique<B> ("osc1FilterHP", "OSC1 Filter HP", false));
    layout.add (std::make_unique<F> ("osc1FilterCutoff", "OSC1 Filter Cut",
        juce::NormalisableRange<float>(30.f,17000.f,1.f,0.3f), 8000.f));

    // ── OSC 2 ─────────────────────────────────────────────────────
    layout.add (std::make_unique<C> ("wave2", "OSC2 Wave",
        juce::StringArray { "Lorenz","FM","Granular","Saw","Square","Drift" }, 2));
    layout.add (std::make_unique<F> ("osc2Vol",    "OSC2 Vol",    0.0f,  1.0f,  0.5f));
    layout.add (std::make_unique<F> ("osc2Detune", "OSC2 Detune",-50.0f,50.0f, -7.0f));
    layout.add (std::make_unique<C> ("osc2Oct", "OSC2 Octave",
        juce::StringArray { "-2","-1","0","+1","+2" }, 2));
    layout.add (std::make_unique<F> ("osc2Pwm",  "OSC2 PWM",   0.05f, 0.95f, 0.3f));
    layout.add (std::make_unique<B> ("osc2On",   "OSC2 On",    true));
    // OSC2 Mini-Filter
    layout.add (std::make_unique<B> ("osc2FilterOn", "OSC2 Filter On", false));
    layout.add (std::make_unique<B> ("osc2FilterHP", "OSC2 Filter HP", false));
    layout.add (std::make_unique<F> ("osc2FilterCutoff", "OSC2 Filter Cut",
        juce::NormalisableRange<float>(30.f,17000.f,1.f,0.3f), 8000.f));

    // ── FILTER ────────────────────────────────────────────────────
    layout.add (std::make_unique<F> ("cutoff", "Cutoff",
        juce::NormalisableRange<float>(30.f,17000.f,1.f,0.3f), 1200.0f));
    layout.add (std::make_unique<F> ("resonance",    "Resonance",  0.0f, 0.85f, 0.3f));
    layout.add (std::make_unique<F> ("drive",        "Drive",      0.0f, 1.0f,  0.1f));
    layout.add (std::make_unique<F> ("filterEnvAmt", "Filter Env", 0.0f, 1.0f,  0.35f));
    layout.add (std::make_unique<C> ("filterType",   "Filter Type",
        juce::StringArray { "LP4","HP4","BP2","Notch" }, 0));

    // ── LFO 1 ─────────────────────────────────────────────────────
    layout.add (std::make_unique<F> ("lfoRate","LFO Rate",
        juce::NormalisableRange<float>(0.05f,12.f,0.01f,0.5f), 0.4f));
    layout.add (std::make_unique<F> ("lfoDepth","LFO Depth", 0.0f,1.0f,0.15f));

    // ── LFO 2 ─────────────────────────────────────────────────────
    layout.add (std::make_unique<F> ("lfo2Rate","LFO2 Rate",
        juce::NormalisableRange<float>(0.05f,20.f,0.01f,0.5f), 0.8f));
    layout.add (std::make_unique<F> ("lfo2Depth",   "LFO2 Depth",   0.0f,1.0f,0.0f));
    layout.add (std::make_unique<C> ("lfo2Target",  "LFO2 Target",
        juce::StringArray { "Pitch","Cutoff","PWM","Morph" }, 1));
    layout.add (std::make_unique<F> ("lfo2Attack",  "LFO2 Attack",  0.001f,4.f,0.2f));
    layout.add (std::make_unique<F> ("lfo2Decay",   "LFO2 Decay",   0.01f, 4.f,0.8f));
    layout.add (std::make_unique<F> ("lfo2Sustain", "LFO2 Sustain", 0.0f,  1.f,0.8f));
    layout.add (std::make_unique<F> ("lfo2Release", "LFO2 Release", 0.01f, 4.f,0.5f));

    // ── Amp ADSR ──────────────────────────────────────────────────
    layout.add (std::make_unique<F> ("attack",  "Attack",  0.001f,3.0f,0.08f));
    layout.add (std::make_unique<F> ("decay",   "Decay",   0.01f, 3.0f,0.6f));
    layout.add (std::make_unique<F> ("sustain", "Sustain", 0.0f,  1.0f,0.55f));
    layout.add (std::make_unique<F> ("release", "Release", 0.01f, 4.0f,0.9f));

    // ── FX ────────────────────────────────────────────────────────
    layout.add (std::make_unique<F> ("chaos",   "Chaos",   0.0f,1.0f,0.12f));
    layout.add (std::make_unique<F> ("chaos2",  "Fractal", 0.0f,1.0f,0.0f));
    layout.add (std::make_unique<F> ("stutter", "Stutter", 0.0f,1.0f,0.0f));
    layout.add (std::make_unique<C> ("stutterMode","Stutter Mode",
        juce::StringArray{"Free","Sync"}, 0));
    layout.add (std::make_unique<C> ("stutterDiv","Stutter Div",
        juce::StringArray{"1/4","1/8","1/16","1/32","1/64","1/128",
        "1/4T","1/8T","1/16T","1/4Q","1/8Q","1/4S","1/8S"}, 2));

    // ── LIVE ──────────────────────────────────────────────────────
    layout.add (std::make_unique<F> ("morph","Morph",
        juce::NormalisableRange<float>(0.f,5.f,0.01f),0.0f));
    layout.add (std::make_unique<F> ("morphAuto", "Morph Auto", 0.0f,1.0f,0.0f));
    layout.add (std::make_unique<F> ("phaseDest", "Phase Dest", 0.0f,1.0f,0.0f));
    layout.add (std::make_unique<F> ("timeFold",  "Time Fold",  0.0f,1.0f,0.0f));

    // ── Granular ──────────────────────────────────────────────────
    layout.add (std::make_unique<F> ("gran1","Granular 1",0.0f,0.6f,0.0f));
    layout.add (std::make_unique<F> ("gran2","Granular 2",0.0f,0.6f,0.0f));

    // ── Portamento + Unison ───────────────────────────────────────
    layout.add (std::make_unique<F> ("portamento","Portamento",
        juce::NormalisableRange<float>(0.f,2.f,0.001f,0.4f),0.0f));
    layout.add (std::make_unique<F> ("unisonDetune","Unison Detune",0.0f,50.0f,0.0f));
    layout.add (std::make_unique<F> ("unisonSpread","Unison Spread",0.0f,1.0f,0.5f));

    // ── Delay ─────────────────────────────────────────────────────
    layout.add (std::make_unique<F> ("delayMix","Delay Mix",0.0f,1.0f,0.18f));
    layout.add (std::make_unique<F> ("delayTime","Delay Time",
        juce::NormalisableRange<float>(0.01f,2.f,0.001f,0.4f),0.375f));
    layout.add (std::make_unique<F> ("delayFeedback","Delay FB",0.0f,0.95f,0.35f));

    // ── Reverb ────────────────────────────────────────────────────
    layout.add (std::make_unique<F> ("reverbMix",  "Reverb Mix",  0.0f,1.0f,0.22f));
    layout.add (std::make_unique<F> ("reverbSize", "Reverb Size", 0.0f,1.0f,0.6f));

    // ── Arpeggiator ───────────────────────────────────────────────
    layout.add (std::make_unique<B> ("arpOn","Arp On",false));
    layout.add (std::make_unique<C> ("arpMode","Arp Mode",
        juce::StringArray{"Up","Down","UpDown","Random"},0));
    layout.add (std::make_unique<C> ("arpDiv","Arp Div",
        juce::StringArray{"1/4","1/8","1/16","1/32"},1));
    layout.add (std::make_unique<C> ("arpOctaves","Arp Oct",
        juce::StringArray{"1","2","3","4"},0));

    // ── OSC 3: Sample ─────────────────────────────────────────────
    layout.add (std::make_unique<B>  ("osc3On",   "OSC3 On",    false));
    layout.add (std::make_unique<F>  ("osc3Vol",  "OSC3 Vol",   0.0f, 1.0f,  0.7f));
    layout.add (std::make_unique<F>  ("smpStart", "Smp Start",  0.0f, 1.0f,  0.0f));
    layout.add (std::make_unique<F>  ("smpEnd",   "Smp End",    0.0f, 1.0f,  1.0f));
    layout.add (std::make_unique<C>  ("smpRoot",  "Smp Root",
        []{ juce::StringArray n; for(int i=0;i<128;++i)
            n.add(juce::MidiMessage::getMidiNoteName(i,true,true,4)); return n; }(), 60));
    layout.add (std::make_unique<C>  ("smpLoop",  "Smp Loop",
        juce::StringArray{"Off","Forward","PingPong"}, 1));
    // OSC3 Mini-Filter
    layout.add (std::make_unique<B> ("smp1FilterOn",  "SMP1 Filter On", false));
    layout.add (std::make_unique<F> ("smp1FilterCutoff","SMP1 Filter Cut",
        juce::NormalisableRange<float>(30.f,17000.f,1.f,0.3f), 8000.f));

    // ── OSC 4: Grain Stutter Sample ───────────────────────────────
    layout.add (std::make_unique<B>  ("osc4On",       "OSC4 On",         false));
    layout.add (std::make_unique<F>  ("osc4Vol",      "OSC4 Vol",        0.0f,  1.0f,   0.7f));
    layout.add (std::make_unique<F>  ("smp2Start",    "SMP2 Start",      0.0f,  1.0f,   0.0f));
    layout.add (std::make_unique<F>  ("smp2End",      "SMP2 End",        0.0f,  1.0f,   1.0f));
    layout.add (std::make_unique<C>  ("smp2Root",     "SMP2 Root",
        []{ juce::StringArray n; for(int i=0;i<128;++i)
            n.add(juce::MidiMessage::getMidiNoteName(i,true,true,4)); return n; }(), 60));
    layout.add (std::make_unique<F>  ("smp2GrainRate","SMP2 Grain Rate",
        juce::NormalisableRange<float>(1.f,64.f,0.1f,0.5f), 8.f));
    layout.add (std::make_unique<F>  ("smp2StutterAcc","SMP2 Stutter Acc",-1.0f,1.0f, 0.0f));
    // OSC4 Mini-Filter
    layout.add (std::make_unique<B> ("smp2FilterOn",  "SMP2 Filter On", false));
    layout.add (std::make_unique<F> ("smp2FilterCutoff","SMP2 Filter Cut",
        juce::NormalisableRange<float>(30.f,17000.f,1.f,0.3f), 8000.f));

    // ── Section Bypass ────────────────────────────────────────────
    layout.add (std::make_unique<B> ("secFilter", "Filter On", true));
    layout.add (std::make_unique<B> ("secFX",     "FX On",     true));
    layout.add (std::make_unique<B> ("secLFO2",   "LFO2 On",   true));
    layout.add (std::make_unique<B> ("monoMode",  "Mono",      false));

    layout.add (std::make_unique<F> ("volume","Volume",0.0f,1.0f,0.78f));

    return layout;
}

Fraktal611AudioProcessor::Fraktal611AudioProcessor()
    : AudioProcessor (BusesProperties()
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameters())
{
    synth.addSound (new SynthSound());
    for (int i=0; i<6; ++i) {
        auto* v = new SynthVoice (waveEngine, apvts);
        v->setSharedSample  (&sharedSample);
        v->setSharedSample2 (&s_sharedSample2);
        synth.addVoice (v);
    }
}

Fraktal611AudioProcessor::~Fraktal611AudioProcessor() {}

void Fraktal611AudioProcessor::prepareToPlay (double sr, int block)
{
    synth.setCurrentPlaybackSampleRate (sr);
    for (int i=0; i<synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
            v->prepareVoice (sr);
    arp.setSampleRate (sr);
    oscBuffer.setSize (1, block);
    scopeBuffer.fill (0.0f);
    scopeWritePos   = 0;
    globalDelayWrite = 0;
    for (int ch=0;ch<2;++ch) {
        std::fill(std::begin(globalDelayLine[ch]),std::end(globalDelayLine[ch]),0.f);
        globalReverbBuf[ch][0]=globalReverbBuf[ch][1]=
        globalReverbBuf[ch][2]=globalReverbBuf[ch][3]=0.f;
    }
}

void Fraktal611AudioProcessor::releaseResources() {}

void Fraktal611AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                   juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    buffer.clear();

    float bpm = 120.0f;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (pos->getBpm().hasValue())
                bpm = static_cast<float>(*pos->getBpm());

    float a=*apvts.getRawParameterValue("attack"),
          d=*apvts.getRawParameterValue("decay"),
          s=*apvts.getRawParameterValue("sustain"),
          r=*apvts.getRawParameterValue("release");
    float l2a=*apvts.getRawParameterValue("lfo2Attack"),
          l2d=*apvts.getRawParameterValue("lfo2Decay"),
          l2s=*apvts.getRawParameterValue("lfo2Sustain"),
          l2r=*apvts.getRawParameterValue("lfo2Release");
    float uniDet=*apvts.getRawParameterValue("unisonDetune");
    float uniSpr=*apvts.getRawParameterValue("unisonSpread");
    int   nv = synth.getNumVoices();

    for (int i=0; i<nv; ++i)
        if (auto* v = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
            v->updateADSR (a,d,s,r);
            v->updateLFO2ADSR (l2a,l2d,l2s,l2r);
            v->setBPM (bpm);
            float t   = (nv>1) ? (float)i/(nv-1) : 0.5f;
            float det = (nv>1) ? (t-0.5f)*2.0f*uniDet : 0.0f;
            float pan = (nv>1) ? (t-0.5f)*2.0f*uniSpr : 0.0f;
            v->setUnisonDetune (det);
            v->setStereoSpread (pan);
        }

    bool arpOn = *apvts.getRawParameterValue("arpOn") > 0.5f;
    arp.setEnabled  (arpOn);
    arp.setMode     ((int)*apvts.getRawParameterValue("arpMode"));
    arp.setDivision ((int)*apvts.getRawParameterValue("arpDiv"));
    arp.setOctaves  (1+(int)*apvts.getRawParameterValue("arpOctaves"));
    arp.setBPM (bpm);

    // Keyboard-MIDI (Maus-Spielen) in den Buffer einfügen
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

    juce::MidiBuffer processed;
    if (arpOn) arp.process (midi, processed, buffer.getNumSamples());
    else       processed = midi;

    // Mono-Modus: neue Note killt vorherige sofort
    if (*apvts.getRawParameterValue("monoMode") > 0.5f) {
        juce::MidiBuffer monoProcessed;
        for (const auto meta : processed) {
            auto msg = meta.getMessage();
            if (msg.isNoteOn()) {
                // Alle anderen Noten stoppen
                monoProcessed.addEvent(juce::MidiMessage::allNotesOff(1), meta.samplePosition);
            }
            monoProcessed.addEvent(msg, meta.samplePosition);
        }
        processed = monoProcessed;
    }

    applyMidiLearnCC (processed);

    synth.renderNextBlock (buffer, processed, 0, buffer.getNumSamples());
    buffer.applyGain (*apvts.getRawParameterValue("volume"));

    // ── Globaler Delay + Reverb (post-mix, alle Voices zusammen) ──
    {
        float delayMix  = *apvts.getRawParameterValue("delayMix");
        float delayTime = *apvts.getRawParameterValue("delayTime");
        float delayFB   = *apvts.getRawParameterValue("delayFeedback");
        float revMix    = *apvts.getRawParameterValue("reverbMix");
        float revSize   = *apvts.getRawParameterValue("reverbSize");

        int delaySmp = juce::jlimit(1, DELAY_BUF_SIZE-1,
                           (int)(delayTime * getSampleRate()));

        int nCh = buffer.getNumChannels();
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            for (int ch = 0; ch < juce::jmin(nCh, 2); ++ch)
            {
                float in  = buffer.getSample(ch, i);
                // Delay
                float dOut = globalDelayLine[ch][(globalDelayWrite - delaySmp
                                + DELAY_BUF_SIZE) & (DELAY_BUF_SIZE-1)];
                globalDelayLine[ch][globalDelayWrite & (DELAY_BUF_SIZE-1)]
                    = in + dOut * delayFB;
                float sig = in + dOut * delayMix;
                // Reverb (Schroeder)
                if (revMix > 0.001f) {
                    float g1 = 0.45f + revSize*0.45f;
                    float g2 = 0.38f + revSize*0.38f;
                    float& rb0 = globalReverbBuf[ch][0];
                    float& rb1 = globalReverbBuf[ch][1];
                    float& rb2 = globalReverbBuf[ch][2];
                    float& rb3 = globalReverbBuf[ch][3];
                    float t0 = sig*0.5f + rb0*g1;  rb0 = sig*0.5f + t0*(g1*0.96f);
                    float t1 = t0*0.4f  + rb1*g2;  rb1 = t0*0.4f  + t1*(g2*0.96f);
                    float t2 = t1*0.35f + rb2*(g1*0.82f); rb2 = t1*0.35f + t2*(g1*0.78f);
                    float t3 = t2*0.3f  + rb3*(g2*0.82f); rb3 = t2*0.3f  + t3*(g2*0.78f);
                    float wet = juce::jlimit(-1.f, 1.f, (t1+t3)*0.5f);
                    sig = sig*(1.f-revMix) + wet*revMix;
                }
                buffer.setSample(ch, i, juce::jlimit(-1.f,1.f,sig));
            }
            ++globalDelayWrite;
        }
    }

    // Scope
    {
        const juce::ScopedLock sl (scopeLock);
        for (int i=0; i<buffer.getNumSamples(); ++i)
        {
            float mono=0.f;
            for (int ch=0; ch<buffer.getNumChannels(); ++ch)
                mono += buffer.getSample(ch,i);
            mono /= juce::jmax(1,buffer.getNumChannels());
            scopeBuffer[scopeWritePos % SCOPE_SIZE] = mono;
            ++scopeWritePos;
        }
    }
}

void Fraktal611AudioProcessor::startMidiLearn (const juce::String& id)
{
    midiLearnParamId = id;
    midiLearnActive  = true;
}

void Fraktal611AudioProcessor::applyMidiLearnCC (const juce::MidiBuffer& midi)
{
    for (const auto meta : midi)
    {
        auto msg = meta.getMessage();
        if (!msg.isController()) continue;
        int cc = msg.getControllerNumber();
        if (midiLearnActive && midiLearnParamId.isNotEmpty())
        {
            ccMap[cc] = midiLearnParamId;
            midiLearnActive  = false;
            midiLearnParamId = {};
        }
        if (ccMap.count(cc))
            if (auto* p = apvts.getParameter(ccMap[cc]))
                p->setValueNotifyingHost((float)msg.getControllerValue()/127.f);
    }
}

void Fraktal611AudioProcessor::getScopeData (std::array<float,SCOPE_SIZE>& out)
{
    const juce::ScopedLock sl (scopeLock);
    int wp = scopeWritePos % SCOPE_SIZE;
    for (int i=0; i<SCOPE_SIZE; ++i)
        out[i] = scopeBuffer[(wp+i)%SCOPE_SIZE];
}

void Fraktal611AudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    auto ccXml = state.getOrCreateChildWithName("MidiCCMap",nullptr);
    for (auto& kv : ccMap) {
        auto el = juce::ValueTree("CC");
        el.setProperty("cc",kv.first,nullptr);
        el.setProperty("param",kv.second,nullptr);
        ccXml.appendChild(el,nullptr);
    }
    auto xml = state.createXml();
    copyXmlToBinary(*xml,dest);
}

void Fraktal611AudioProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary(data,size);
    if (xml && xml->hasTagName(apvts.state.getType())) {
        auto tree = juce::ValueTree::fromXml(*xml);
        apvts.replaceState(tree);
        ccMap.clear();
        auto ccXml = tree.getChildWithName("MidiCCMap");
        for (int i=0; i<ccXml.getNumChildren(); ++i) {
            auto el = ccXml.getChild(i);
            int cc=(int)el.getProperty("cc");
            juce::String p=el.getProperty("param").toString();
            if (cc>=0 && p.isNotEmpty()) ccMap[cc]=p;
        }
    }
}

void Fraktal611AudioProcessor::panic()
{
    synth.allNotesOff(0, true);
    keyboardState.allNotesOff(0);
    // Globale Delay/Reverb-Buffer nullen
    for (int ch=0;ch<2;++ch) {
        std::fill(std::begin(globalDelayLine[ch]),std::end(globalDelayLine[ch]),0.f);
        globalReverbBuf[ch][0]=globalReverbBuf[ch][1]=
        globalReverbBuf[ch][2]=globalReverbBuf[ch][3]=0.f;
    }
}

SharedSampleBuffer& Fraktal611AudioProcessor::getSharedSample2()
{
    return s_sharedSample2;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Fraktal611AudioProcessor();
}
