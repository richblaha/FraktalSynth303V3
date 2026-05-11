#include "PluginProcessor.h"
#include "PluginEditor.h"

static SharedSampleBuffer s_sharedSample2;

juce::AudioProcessorValueTreeState::ParameterLayout
Fraktal611AudioProcessor::createParameters()
{
    using F=juce::AudioParameterFloat;
    using C=juce::AudioParameterChoice;
    using B=juce::AudioParameterBool;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // OSC 1 + 2  (8 Wellenformen: 0-5 original, 6=Lorenz Live, 7=Weierstrass)
    const juce::StringArray waveNames{"Lorenz","FM","Granular","Saw","Square","Drift","Lorenz Live","Weierstrass"};
    layout.add(std::make_unique<C>("wave","OSC1 Wave",waveNames,0));
    layout.add(std::make_unique<F>("pwm","PWM",0.05f,0.95f,0.5f));
    layout.add(std::make_unique<F>("osc1Vol","OSC1 Vol",0.0f,1.0f,0.8f));
    layout.add(std::make_unique<C>("osc1Oct","OSC1 Octave",juce::StringArray{"-2","-1","0","+1","+2"},2));
    layout.add(std::make_unique<B>("osc1On","OSC1 On",true));
    layout.add(std::make_unique<B>("osc1FilterOn","OSC1 Filter On",false));
    layout.add(std::make_unique<B>("osc1FilterHP","OSC1 Filter HP",false));
    layout.add(std::make_unique<F>("osc1FilterCutoff","OSC1 Filter Cut",juce::NormalisableRange<float>(30.f,17000.f,1.f,0.3f),8000.f));
    layout.add(std::make_unique<C>("wave2","OSC2 Wave",waveNames,2));
    layout.add(std::make_unique<F>("osc2Vol","OSC2 Vol",0.0f,1.0f,0.5f));
    layout.add(std::make_unique<F>("osc2Detune","OSC2 Detune",-50.0f,50.0f,-7.0f));
    layout.add(std::make_unique<C>("osc2Oct","OSC2 Octave",juce::StringArray{"-2","-1","0","+1","+2"},2));
    layout.add(std::make_unique<F>("osc2Pwm","OSC2 PWM",0.05f,0.95f,0.3f));
    layout.add(std::make_unique<B>("osc2On","OSC2 On",true));
    layout.add(std::make_unique<B>("osc2FilterOn","OSC2 Filter On",false));
    layout.add(std::make_unique<B>("osc2FilterHP","OSC2 Filter HP",false));
    layout.add(std::make_unique<F>("osc2FilterCutoff","OSC2 Filter Cut",juce::NormalisableRange<float>(30.f,17000.f,1.f,0.3f),8000.f));

    // Filter
    layout.add(std::make_unique<F>("cutoff","Cutoff",juce::NormalisableRange<float>(30.f,17000.f,1.f,0.3f),1200.0f));
    layout.add(std::make_unique<F>("resonance","Resonance",0.0f,0.85f,0.3f));
    layout.add(std::make_unique<F>("drive","Drive",0.0f,1.0f,0.1f));
    layout.add(std::make_unique<F>("filterEnvAmt","Filter Env",0.0f,1.0f,0.35f));
    layout.add(std::make_unique<C>("filterType","Filter Type",juce::StringArray{"LP4","HP4","BP2","Notch"},0));

    // LFO 1
    layout.add(std::make_unique<F>("lfoRate","LFO Rate",juce::NormalisableRange<float>(0.05f,12.f,0.01f,0.5f),0.4f));
    layout.add(std::make_unique<F>("lfoDepth","LFO Depth",0.0f,1.0f,0.15f));
    // Poly-temporaler Multiplikator fuer LFO1 (phi, pi, sqrt2, sqrt3, e)
    layout.add(std::make_unique<C>("lfoRateMult","LFO1 Mult",juce::StringArray{"x1","xPhi","xPi","xSqrt2","xSqrt3","xe"},0));

    // LFO 2
    layout.add(std::make_unique<F>("lfo2Rate","LFO2 Rate",juce::NormalisableRange<float>(0.05f,20.f,0.01f,0.5f),0.8f));
    layout.add(std::make_unique<F>("lfo2Depth","LFO2 Depth",0.0f,1.0f,0.0f));
    layout.add(std::make_unique<C>("lfo2Target","LFO2 Target",juce::StringArray{"Pitch","Cutoff","PWM","Morph","LFO1 Rate"},1));
    layout.add(std::make_unique<C>("lfo2RateMult","LFO2 Mult",juce::StringArray{"x1","xPhi","xPi","xSqrt2","xSqrt3","xe"},0));
    layout.add(std::make_unique<F>("lfo2Attack","LFO2 Attack",0.001f,4.f,0.2f));
    layout.add(std::make_unique<F>("lfo2Decay","LFO2 Decay",0.01f,4.f,0.8f));
    layout.add(std::make_unique<F>("lfo2Sustain","LFO2 Sustain",0.0f,1.f,0.8f));
    layout.add(std::make_unique<F>("lfo2Release","LFO2 Release",0.01f,4.f,0.5f));

    // ADSR
    layout.add(std::make_unique<F>("attack","Attack",0.001f,3.0f,0.08f));
    layout.add(std::make_unique<F>("decay","Decay",0.01f,3.0f,0.6f));
    layout.add(std::make_unique<F>("sustain","Sustain",0.0f,1.0f,0.55f));
    layout.add(std::make_unique<F>("release","Release",0.01f,4.0f,0.9f));

    // FX
    layout.add(std::make_unique<F>("chaos","Chaos",0.0f,1.0f,0.12f));
    layout.add(std::make_unique<F>("chaos2","Fractal",0.0f,1.0f,0.0f));
    layout.add(std::make_unique<F>("stutter","Stutter",0.0f,1.0f,0.0f));
    layout.add(std::make_unique<C>("stutterMode","Stutter Mode",juce::StringArray{"Free","Sync"},0));
    layout.add(std::make_unique<C>("stutterDiv","Stutter Div",juce::StringArray{"1/4","1/8","1/16","1/32","1/64","1/128","1/4T","1/8T","1/16T","1/4Q","1/8Q","1/4S","1/8S"},2));

    // Live
    layout.add(std::make_unique<F>("morph","Morph",juce::NormalisableRange<float>(0.f,5.f,0.01f),0.0f));
    layout.add(std::make_unique<F>("morphAuto","Morph Auto",0.0f,1.0f,0.0f));
    layout.add(std::make_unique<F>("phaseDest","Phase Dest",0.0f,1.0f,0.0f));
    layout.add(std::make_unique<F>("timeFold","Time Fold",0.0f,1.0f,0.0f));
    layout.add(std::make_unique<F>("gran1","Granular 1",0.0f,0.6f,0.0f));
    layout.add(std::make_unique<F>("gran2","Granular 2",0.0f,0.6f,0.0f));

    // Porta + Unison
    layout.add(std::make_unique<F>("portamento","Portamento",juce::NormalisableRange<float>(0.f,2.f,0.001f,0.4f),0.0f));
    layout.add(std::make_unique<F>("unisonDetune","Unison Detune",0.0f,50.0f,0.0f));
    layout.add(std::make_unique<F>("unisonSpread","Unison Spread",0.0f,1.0f,0.5f));

    // Delay + Reverb
    layout.add(std::make_unique<F>("delayMix","Delay Mix",0.0f,1.0f,0.18f));
    layout.add(std::make_unique<F>("delayTime","Delay Time",juce::NormalisableRange<float>(0.01f,2.f,0.001f,0.4f),0.375f));
    layout.add(std::make_unique<F>("delayFeedback","Delay FB",0.0f,0.95f,0.35f));
    layout.add(std::make_unique<F>("reverbMix","Reverb Mix",0.0f,1.0f,0.22f));
    layout.add(std::make_unique<F>("reverbSize","Reverb Size",0.0f,1.0f,0.6f));

    // Arpeggiator
    layout.add(std::make_unique<B>("arpOn","Arp On",false));
    layout.add(std::make_unique<C>("arpMode","Arp Mode",juce::StringArray{"Up","Down","UpDown","Random"},0));
    layout.add(std::make_unique<C>("arpDiv","Arp Div",juce::StringArray{"1/4","1/8","1/16","1/32"},1));
    layout.add(std::make_unique<C>("arpOctaves","Arp Oct",juce::StringArray{"1","2","3","4"},0));

    // OSC 3: Sample
    layout.add(std::make_unique<B>("osc3On","OSC3 On",false));
    layout.add(std::make_unique<F>("osc3Vol","OSC3 Vol",0.0f,1.0f,0.7f));
    layout.add(std::make_unique<F>("smpStart","Smp Start",0.0f,1.0f,0.0f));
    layout.add(std::make_unique<F>("smpEnd","Smp End",0.0f,1.0f,1.0f));
    layout.add(std::make_unique<C>("smpRoot","Smp Root",[]{juce::StringArray n;for(int i=0;i<128;++i)n.add(juce::MidiMessage::getMidiNoteName(i,true,true,4));return n;}(),60));
    layout.add(std::make_unique<C>("smpLoop","Smp Loop",juce::StringArray{"Off","Forward","PingPong"},1));
    layout.add(std::make_unique<B>("smp1FilterOn","SMP1 Filter On",false));
    layout.add(std::make_unique<F>("smp1FilterCutoff","SMP1 Filter Cut",juce::NormalisableRange<float>(30.f,17000.f,1.f,0.3f),8000.f));
    layout.add(std::make_unique<F>("smpPitchOct","SMP1 Pitch Oct",juce::NormalisableRange<float>(-24.f,24.f,0.01f),0.f));

    // OSC 4: Grain Stutter
    layout.add(std::make_unique<B>("osc4On","OSC4 On",false));
    layout.add(std::make_unique<F>("osc4Vol","OSC4 Vol",0.0f,1.0f,0.7f));
    layout.add(std::make_unique<F>("smp2Start","SMP2 Start",0.0f,1.0f,0.0f));
    layout.add(std::make_unique<F>("smp2End","SMP2 End",0.0f,1.0f,1.0f));
    layout.add(std::make_unique<C>("smp2Root","SMP2 Root",[]{juce::StringArray n;for(int i=0;i<128;++i)n.add(juce::MidiMessage::getMidiNoteName(i,true,true,4));return n;}(),60));
    layout.add(std::make_unique<F>("smp2GrainRate","SMP2 Grain Rate",juce::NormalisableRange<float>(0.5f,500.f,0.1f,0.35f),8.f));
    layout.add(std::make_unique<F>("smp2StutterAcc","SMP2 Stutter Acc",-3.0f,3.0f,0.0f));
    layout.add(std::make_unique<B>("smp2Reverse","SMP2 Reverse",false));
    layout.add(std::make_unique<B>("smp2FilterOn","SMP2 Filter On",false));
    layout.add(std::make_unique<F>("smp2FilterCutoff","SMP2 Filter Cut",juce::NormalisableRange<float>(30.f,17000.f,1.f,0.3f),8000.f));
    layout.add(std::make_unique<F>("smp2PitchOct","SMP2 Pitch Oct",juce::NormalisableRange<float>(-24.f,24.f,0.01f),0.f));

    // Section bypass + mono
    layout.add(std::make_unique<B>("secFilter","Filter On",true));
    layout.add(std::make_unique<B>("secFX","FX On",true));
    layout.add(std::make_unique<B>("secLFO2","LFO2 On",true));
    layout.add(std::make_unique<B>("monoMode","Mono",false));
    layout.add(std::make_unique<F>("volume","Volume",0.0f,1.0f,0.78f));

    // ── NEU: Lorenz Live Parameter ─────────────────────────────────
    // Sigma/Rho/Beta bestimmen das Attraktor-Verhalten.
    // Rho=24.74: erster Bifurkationspunkt (System kippt von periodisch zu chaotisch)
    layout.add(std::make_unique<F>("lorenzSigma","Lorenz Sigma",juce::NormalisableRange<float>(1.f,25.f,0.01f),10.f));
    layout.add(std::make_unique<F>("lorenzRho","Lorenz Rho",juce::NormalisableRange<float>(0.5f,50.f,0.01f),28.f));
    layout.add(std::make_unique<F>("lorenzBeta","Lorenz Beta",juce::NormalisableRange<float>(0.1f,8.f,0.01f),2.667f));
    layout.add(std::make_unique<F>("lorenzDt","Lorenz dt",juce::NormalisableRange<float>(0.0001f,0.005f,0.0001f,0.3f),0.002f));

    // ── NEU: Weierstrass Parameter ────────────────────────────────
    layout.add(std::make_unique<F>("weiDimension","Wei Dim",juce::NormalisableRange<float>(1.0f,1.99f,0.01f),1.5f));
    layout.add(std::make_unique<C>("weiLacunarity","Wei Lac",juce::StringArray{"2","3","5","7"},1));
    layout.add(std::make_unique<C>("weiDepth","Wei Depth",juce::StringArray{"2","3","4","5","6","7","8","9","10","11","12"},4));

    // ── NEU: Gray-Scott Oszillator ────────────────────────────────
    layout.add(std::make_unique<B>("gsOn","GS On",false));
    layout.add(std::make_unique<F>("gsFeed","GS Feed",juce::NormalisableRange<float>(0.010f,0.090f,0.001f),0.055f));
    layout.add(std::make_unique<F>("gsKill","GS Kill",juce::NormalisableRange<float>(0.040f,0.070f,0.001f),0.062f));
    layout.add(std::make_unique<F>("gsDa","GS Da",juce::NormalisableRange<float>(0.1f,1.0f,0.01f),1.0f));
    layout.add(std::make_unique<F>("gsDb","GS Db",juce::NormalisableRange<float>(0.01f,0.8f,0.01f),0.5f));
    layout.add(std::make_unique<F>("gsVol","GS Vol",0.0f,1.0f,0.7f));

    // ── NEU: Evolutionary Drift ───────────────────────────────────
    layout.add(std::make_unique<B>("driftOn","Drift On",false));
    layout.add(std::make_unique<F>("driftSpeed","Drift Speed",juce::NormalisableRange<float>(0.001f,1.0f,0.001f,0.3f),0.05f));
    layout.add(std::make_unique<F>("driftRange","Drift Range",0.01f,1.0f,0.15f));
    layout.add(std::make_unique<C>("driftMode","Drift Mode",juce::StringArray{"Brownian","Orbital","Avalanche"},0));

    return layout;
}

Fraktal611AudioProcessor::Fraktal611AudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),
      apvts(*this,nullptr,"Parameters",createParameters())
{
    synth.addSound(new SynthSound());
    for(int i=0;i<6;++i){auto* v=new SynthVoice(waveEngine,apvts);v->setSharedSample(&sharedSample);v->setSharedSample2(&s_sharedSample2);synth.addVoice(v);}
    drift.prepare(apvts);
}

Fraktal611AudioProcessor::~Fraktal611AudioProcessor(){}

void Fraktal611AudioProcessor::prepareToPlay(double sr,int block)
{
    synth.setCurrentPlaybackSampleRate(sr);
    for(int i=0;i<synth.getNumVoices();++i)if(auto* v=dynamic_cast<SynthVoice*>(synth.getVoice(i)))v->prepareVoice(sr);
    arp.setSampleRate(sr);
    oscBuffer.setSize(1,block);
    scopeBuffer.fill(0.0f);scopeWritePos=0;globalDelayWrite=0;
    for(int ch=0;ch<2;++ch){std::fill(std::begin(globalDelayLine[ch]),std::end(globalDelayLine[ch]),0.f);globalReverbBuf[ch][0]=globalReverbBuf[ch][1]=globalReverbBuf[ch][2]=globalReverbBuf[ch][3]=0.f;}
}

void Fraktal611AudioProcessor::releaseResources(){}

void Fraktal611AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    buffer.clear();
    float bpm=120.0f;
    if(auto* ph=getPlayHead())if(auto pos=ph->getPosition())if(pos->getBpm().hasValue())bpm=static_cast<float>(*pos->getBpm());
    float a=*apvts.getRawParameterValue("attack"),d=*apvts.getRawParameterValue("decay"),s=*apvts.getRawParameterValue("sustain"),r=*apvts.getRawParameterValue("release");
    float l2a=*apvts.getRawParameterValue("lfo2Attack"),l2d=*apvts.getRawParameterValue("lfo2Decay"),l2s=*apvts.getRawParameterValue("lfo2Sustain"),l2r=*apvts.getRawParameterValue("lfo2Release");
    float uniDet=*apvts.getRawParameterValue("unisonDetune"),uniSpr=*apvts.getRawParameterValue("unisonSpread");
    int nv=synth.getNumVoices();
    for(int i=0;i<nv;++i)if(auto* v=dynamic_cast<SynthVoice*>(synth.getVoice(i))){v->updateADSR(a,d,s,r);v->updateLFO2ADSR(l2a,l2d,l2s,l2r);v->setBPM(bpm);float t=(nv>1)?(float)i/(nv-1):0.5f;float det=(nv>1)?(t-0.5f)*2.0f*uniDet:0.0f;float pan=(nv>1)?(t-0.5f)*2.0f*uniSpr:0.0f;v->setUnisonDetune(det);v->setStereoSpread(pan);}
    bool arpOn=*apvts.getRawParameterValue("arpOn")>0.5f;
    arp.setEnabled(arpOn);arp.setMode((int)*apvts.getRawParameterValue("arpMode"));arp.setDivision((int)*apvts.getRawParameterValue("arpDiv"));arp.setOctaves(1+(int)*apvts.getRawParameterValue("arpOctaves"));arp.setBPM(bpm);
    keyboardState.processNextMidiBuffer(midi,0,buffer.getNumSamples(),true);
    juce::MidiBuffer processed;
    if(arpOn)arp.process(midi,processed,buffer.getNumSamples());else processed=midi;
    if(*apvts.getRawParameterValue("monoMode")>0.5f){juce::MidiBuffer mp;for(const auto meta:processed){auto msg=meta.getMessage();if(msg.isNoteOn())mp.addEvent(juce::MidiMessage::allNotesOff(1),meta.samplePosition);mp.addEvent(msg,meta.samplePosition);}processed=mp;}
    applyMidiLearnCC(processed);
    synth.renderNextBlock(buffer,processed,0,buffer.getNumSamples());
    buffer.applyGain(*apvts.getRawParameterValue("volume"));
    // Delay + Reverb
    {float dMix=*apvts.getRawParameterValue("delayMix"),dTime=*apvts.getRawParameterValue("delayTime"),dFB=*apvts.getRawParameterValue("delayFeedback"),rMix=*apvts.getRawParameterValue("reverbMix"),rSz=*apvts.getRawParameterValue("reverbSize");
    int dSmp=juce::jlimit(1,DELAY_BUF_SIZE-1,(int)(dTime*getSampleRate()));int nCh=buffer.getNumChannels();
    for(int i=0;i<buffer.getNumSamples();++i){for(int ch=0;ch<juce::jmin(nCh,2);++ch){float in=buffer.getSample(ch,i);float dOut=globalDelayLine[ch][(globalDelayWrite-dSmp+DELAY_BUF_SIZE)&(DELAY_BUF_SIZE-1)];globalDelayLine[ch][globalDelayWrite&(DELAY_BUF_SIZE-1)]=in+dOut*dFB;float sig=in+dOut*dMix;if(rMix>0.001f){float g1=0.45f+rSz*0.45f,g2=0.38f+rSz*0.38f;float&rb0=globalReverbBuf[ch][0],&rb1=globalReverbBuf[ch][1],&rb2=globalReverbBuf[ch][2],&rb3=globalReverbBuf[ch][3];float t0=sig*0.5f+rb0*g1;rb0=sig*0.5f+t0*(g1*0.96f);float t1=t0*0.4f+rb1*g2;rb1=t0*0.4f+t1*(g2*0.96f);float t2=t1*0.35f+rb2*(g1*0.82f);rb2=t1*0.35f+t2*(g1*0.78f);float t3=t2*0.3f+rb3*(g2*0.82f);rb3=t2*0.3f+t3*(g2*0.78f);float wet=juce::jlimit(-1.f,1.f,(t1+t3)*0.5f);sig=sig*(1.f-rMix)+wet*rMix;}buffer.setSample(ch,i,juce::jlimit(-1.f,1.f,sig));}++globalDelayWrite;}}
    // Scope
    {const juce::ScopedLock sl(scopeLock);for(int i=0;i<buffer.getNumSamples();++i){float mono=0.f;for(int ch=0;ch<buffer.getNumChannels();++ch)mono+=buffer.getSample(ch,i);mono/=juce::jmax(1,buffer.getNumChannels());scopeBuffer[scopeWritePos%SCOPE_SIZE]=mono;++scopeWritePos;}}
    // Evolutionary Drift
    drift.enabled=*apvts.getRawParameterValue("driftOn")>0.5f;
    drift.process(buffer.getNumSamples(),*apvts.getRawParameterValue("driftSpeed"),*apvts.getRawParameterValue("driftRange"),(int)*apvts.getRawParameterValue("driftMode"));
}

void Fraktal611AudioProcessor::startMidiLearn(const juce::String& id){midiLearnParamId=id;midiLearnActive=true;}
void Fraktal611AudioProcessor::applyMidiLearnCC(const juce::MidiBuffer& midi){for(const auto meta:midi){auto msg=meta.getMessage();if(!msg.isController())continue;int cc=msg.getControllerNumber();if(midiLearnActive&&midiLearnParamId.isNotEmpty()){ccMap[cc]=midiLearnParamId;midiLearnActive=false;midiLearnParamId={};}if(ccMap.count(cc))if(auto* p=apvts.getParameter(ccMap[cc]))p->setValueNotifyingHost((float)msg.getControllerValue()/127.f);}}
void Fraktal611AudioProcessor::getScopeData(std::array<float,SCOPE_SIZE>& out){const juce::ScopedLock sl(scopeLock);int wp=scopeWritePos%SCOPE_SIZE;for(int i=0;i<SCOPE_SIZE;++i)out[i]=scopeBuffer[(wp+i)%SCOPE_SIZE];}
void Fraktal611AudioProcessor::getStateInformation(juce::MemoryBlock& dest){auto state=apvts.copyState();auto ccXml=state.getOrCreateChildWithName("MidiCCMap",nullptr);for(auto& kv:ccMap){auto el=juce::ValueTree("CC");el.setProperty("cc",kv.first,nullptr);el.setProperty("param",kv.second,nullptr);ccXml.appendChild(el,nullptr);}auto xml=state.createXml();copyXmlToBinary(*xml,dest);}
void Fraktal611AudioProcessor::setStateInformation(const void* data,int size){auto xml=getXmlFromBinary(data,size);if(xml&&xml->hasTagName(apvts.state.getType())){auto tree=juce::ValueTree::fromXml(*xml);apvts.replaceState(tree);ccMap.clear();auto ccXml=tree.getChildWithName("MidiCCMap");for(int i=0;i<ccXml.getNumChildren();++i){auto el=ccXml.getChild(i);int cc=(int)el.getProperty("cc");juce::String p=el.getProperty("param").toString();if(cc>=0&&p.isNotEmpty())ccMap[cc]=p;}}}
void Fraktal611AudioProcessor::panic(){synth.allNotesOff(0,true);keyboardState.allNotesOff(0);for(int ch=0;ch<2;++ch){std::fill(std::begin(globalDelayLine[ch]),std::end(globalDelayLine[ch]),0.f);globalReverbBuf[ch][0]=globalReverbBuf[ch][1]=globalReverbBuf[ch][2]=globalReverbBuf[ch][3]=0.f;}}
SharedSampleBuffer& Fraktal611AudioProcessor::getSharedSample2(){return s_sharedSample2;}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Fraktal611AudioProcessor();}
