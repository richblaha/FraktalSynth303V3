#pragma once
#include <JuceHeader.h>
#include "WaveEngine.h"
#include "LadderFilter.h"
#include "SampleOscillator.h"
#include "GrayScottOscillator.h"
#include <cmath>
#include <array>

class MiniFilter
{
public:
    void setSampleRate(double sr){sampleRate=sr;}
    void setLP(float c){float w=juce::MathConstants<float>::twoPi*c/(float)sampleRate;a=w/(w+1.f);type=0;}
    void setHP(float c){float w=juce::MathConstants<float>::twoPi*c/(float)sampleRate;a=1.f/(w+1.f);type=1;}
    void setBypassed(bool b){bypassed=b;}
    float processL(float x){if(bypassed)return x;if(type==0){z1L=a*x+(1.f-a)*z1L;return z1L;}float y=a*(z1L+x-prevL);prevL=x;z1L=y;return y;}
    float processR(float x){if(bypassed)return x;if(type==0){z1R=a*x+(1.f-a)*z1R;return z1R;}float y=a*(z1R+x-prevR);prevR=x;z1R=y;return y;}
    void reset(){z1L=z1R=prevL=prevR=0.f;}
private:
    double sampleRate=44100.0;
    float a=1.f,z1L=0.f,z1R=0.f,prevL=0.f,prevR=0.f;
    int type=0;bool bypassed=true;
};

class SynthVoice : public juce::SynthesiserVoice
{
public:
    SynthVoice(const WaveEngine& we,juce::AudioProcessorValueTreeState& apvts):waveEngine(we),tree(apvts){}
    bool canPlaySound(juce::SynthesiserSound* s) override{return dynamic_cast<juce::SynthesiserSound*>(s)!=nullptr;}
    void setBPM(float b){bpm=b;}
    void setUnisonDetune(float c){unisonDetuneCents=c;}
    void setStereoSpread(float p){stereoPan=juce::jlimit(-1.f,1.f,p);}
    void setSharedSample(const SharedSampleBuffer* b){samplePlayer.setBuffer(b);}
    void setSharedSample2(const SharedSampleBuffer* b){grainPlayer.setBuffer(b);}

    void startNote(int midi,float vel,juce::SynthesiserSound*,int) override
    {
        targetFreq=juce::MidiMessage::getMidiNoteInHertz(midi);
        if(currentFreq<=0.0)currentFreq=targetFreq;
        midiNote=midi;
        phase=lfoPhase=lfo2Phase=stutterPhase=flangerPhase=0.0;
        shimmerPhase=morphAutoPhase=phaseDestPhase=0.0;
        osc2Phase=0.0;
        driftPhase1=juce::Random::getSystemRandom().nextFloat()*6.28f;
        driftPhase2=juce::Random::getSystemRandom().nextFloat()*6.28f;
        gran1Phase=gran2Phase=grainPhase=0.f;
        gran1Pos=juce::Random::getSystemRandom().nextFloat();
        gran2Pos=juce::Random::getSystemRandom().nextFloat();
        grainPos=juce::Random::getSystemRandom().nextFloat();
        osc2GranPos=juce::Random::getSystemRandom().nextFloat();
        osc2GranPhase=0.f;
        smoothedGate=pdFeedback=0.f;
        velocity=vel;
        auto& r=juce::Random::getSystemRandom();
        lorenzLiveX=0.1f+r.nextFloat()*0.2f; lorenzLiveY=r.nextFloat()*0.2f; lorenzLiveZ=20.f+r.nextFloat()*10.f;
        lorenzLive2X=-(0.1f+r.nextFloat()*0.2f); lorenzLive2Y=-(r.nextFloat()*0.2f); lorenzLive2Z=25.f+r.nextFloat()*10.f;
        timeFoldBuf.fill(0.f);timeFoldPos=0;
        std::fill(std::begin(flangerBuf),std::end(flangerBuf),0.f);flangerWrite=0;
        ampEnv.noteOn();filterEnv.noteOn();lfo2Env.noteOn();
        filter.reset();miniFilter1.reset();miniFilter2.reset();miniFilterS1.reset();miniFilterS2.reset();
        if(*tree.getRawParameterValue("osc3On")>0.5f&&samplePlayer.hasFile()) samplePlayer.noteOn(midi,vel);
        if(*tree.getRawParameterValue("osc4On")>0.5f&&grainPlayer.hasFile()){
            grainPlayer.baseRate=*tree.getRawParameterValue("smp2GrainRate");
            grainPlayer.stutterAcc=*tree.getRawParameterValue("smp2StutterAcc");
            grainPlayer.startPt=*tree.getRawParameterValue("smp2Start");
            grainPlayer.endPt=*tree.getRawParameterValue("smp2End");
            grainPlayer.rootNote=(int)*tree.getRawParameterValue("smp2Root");
            grainPlayer.volume=*tree.getRawParameterValue("osc4Vol");
            grainPlayer.reverseMode=*tree.getRawParameterValue("smp2Reverse")>0.5f;
            grainPlayer.noteOn(midi,vel);
        }
        if(*tree.getRawParameterValue("gsOn")>0.5f) gsOsc.noteOn(midi,vel);
        else gsOsc.setActive(false);
    }

    void stopNote(float,bool tail) override
    {
        ampEnv.noteOff();filterEnv.noteOff();lfo2Env.noteOff();
        samplePlayer.noteOff();grainPlayer.noteOff();
        if(!tail)clearCurrentNote();
    }

    void pitchWheelMoved(int v) override{pitchBendSemitones=(float)v/8192.f*2.f;}
    void controllerMoved(int cc,int v) override{if(cc==1)modWheelValue=(float)v/127.f;}
    void updateADSR(float a,float d,float s,float r){ampEnv.setParameters({a,d,s,r});}
    void updateLFO2ADSR(float a,float d,float s,float r){lfo2Env.setParameters({a,d,s,r});}

    void prepareVoice(double sr)
    {
        filter.setSampleRate(sr);
        miniFilter1.setSampleRate(sr);miniFilter2.setSampleRate(sr);
        miniFilterS1.setSampleRate(sr);miniFilterS2.setSampleRate(sr);
        ampEnv.setSampleRate(sr);ampEnv.setParameters({0.08f,0.6f,0.55f,0.9f});
        filterEnv.setSampleRate(sr);filterEnv.setParameters({0.002f,0.2f,0.f,0.1f});
        lfo2Env.setSampleRate(sr);lfo2Env.setParameters({0.2f,0.8f,1.0f,0.5f});
        grainPlayer.setSampleRate(sr);
        gsOsc.setSampleRate(sr);
        std::fill(std::begin(flangerBuf),std::end(flangerBuf),0.f);
        flangerWrite=0;timeFoldBuf.fill(0.f);timeFoldPos=0;pdFeedback=0.f;
    }

    void renderNextBlock(juce::AudioBuffer<float>& out,int start,int num) override
    {
        if(!ampEnv.isActive())return;
        const int   w1=(int)*tree.getRawParameterValue("wave");
        const float pwm=*tree.getRawParameterValue("pwm");
        const float osc1Vol=*tree.getRawParameterValue("osc1Vol");
        const int   osc1Oct=(int)*tree.getRawParameterValue("osc1Oct")-2;
        const bool  osc1On=*tree.getRawParameterValue("osc1On")>0.5f;
        const int   w2=(int)*tree.getRawParameterValue("wave2");
        const float osc2Vol=*tree.getRawParameterValue("osc2Vol");
        const float osc2Det=*tree.getRawParameterValue("osc2Detune");
        const int   osc2Oct=(int)*tree.getRawParameterValue("osc2Oct")-2;
        const float osc2Pwm=*tree.getRawParameterValue("osc2Pwm");
        const bool  osc2On=*tree.getRawParameterValue("osc2On")>0.5f;
        const bool  osc3On=*tree.getRawParameterValue("osc3On")>0.5f;
        const float osc3Vol=*tree.getRawParameterValue("osc3Vol");
        const float smpStart=*tree.getRawParameterValue("smpStart");
        const float smpEnd=*tree.getRawParameterValue("smpEnd");
        const int   smpRoot=(int)*tree.getRawParameterValue("smpRoot");
        const int   smpLoop=(int)*tree.getRawParameterValue("smpLoop");
        const bool  osc4On=*tree.getRawParameterValue("osc4On")>0.5f;
        const float osc4Vol=*tree.getRawParameterValue("osc4Vol");
        const float s2GrRate=*tree.getRawParameterValue("smp2GrainRate");
        const float s2StAcc=*tree.getRawParameterValue("smp2StutterAcc");
        const float s2Start=*tree.getRawParameterValue("smp2Start");
        const float s2End=*tree.getRawParameterValue("smp2End");
        const int   s2Root=(int)*tree.getRawParameterValue("smp2Root");
        const float smpPitchOct=*tree.getRawParameterValue("smpPitchOct");
        const float smp2PitchOct=*tree.getRawParameterValue("smp2PitchOct");
        const bool  smp2Rev=*tree.getRawParameterValue("smp2Reverse")>0.5f;
        const int   effectiveSmpRoot=smpRoot-(int)std::round(smpPitchOct*12.f);
        const int   effectiveSmp2Root=s2Root-(int)std::round(smp2PitchOct*12.f);
        const float osc1FC=*tree.getRawParameterValue("osc1FilterCutoff");
        const bool  osc1FOn=*tree.getRawParameterValue("osc1FilterOn")>0.5f;
        const bool  osc1FHP=*tree.getRawParameterValue("osc1FilterHP")>0.5f;
        const float osc2FC=*tree.getRawParameterValue("osc2FilterCutoff");
        const bool  osc2FOn=*tree.getRawParameterValue("osc2FilterOn")>0.5f;
        const bool  osc2FHP=*tree.getRawParameterValue("osc2FilterHP")>0.5f;
        const float s1FC=*tree.getRawParameterValue("smp1FilterCutoff");
        const bool  s1FOn=*tree.getRawParameterValue("smp1FilterOn")>0.5f;
        const float s2FC=*tree.getRawParameterValue("smp2FilterCutoff");
        const bool  s2FOn=*tree.getRawParameterValue("smp2FilterOn")>0.5f;
        const bool  filterOn=*tree.getRawParameterValue("secFilter")>0.5f;
        const bool  fxOn=*tree.getRawParameterValue("secFX")>0.5f;
        const bool  lfo2On=*tree.getRawParameterValue("secLFO2")>0.5f;
        const float gran1Mix=*tree.getRawParameterValue("gran1");
        const float gran2Mix=*tree.getRawParameterValue("gran2");
        const float cutoff=*tree.getRawParameterValue("cutoff");
        const float resonance=*tree.getRawParameterValue("resonance");
        const float drive=*tree.getRawParameterValue("drive");
        const float envAmt=*tree.getRawParameterValue("filterEnvAmt");
        const int   ftype=(int)*tree.getRawParameterValue("filterType");
        const float lfoRate=*tree.getRawParameterValue("lfoRate");
        const float lfoDepth=*tree.getRawParameterValue("lfoDepth")+modWheelValue*0.5f;
        const float lfo2Rate=*tree.getRawParameterValue("lfo2Rate");
        const float lfo2Depth=*tree.getRawParameterValue("lfo2Depth");
        const int   lfo2Tgt=(int)*tree.getRawParameterValue("lfo2Target");
        const float chaos=juce::jlimit(0.f,0.8f,(float)*tree.getRawParameterValue("chaos"));
        const float chaos2=*tree.getRawParameterValue("chaos2");
        const float stutterA=*tree.getRawParameterValue("stutter");
        const float morphPos0=*tree.getRawParameterValue("morph");
        const float morphAuto=*tree.getRawParameterValue("morphAuto");
        const float phaseDest=*tree.getRawParameterValue("phaseDest");
        const float timeFold=*tree.getRawParameterValue("timeFold");
        const float porta=*tree.getRawParameterValue("portamento");
        const int   stuttDiv=(int)*tree.getRawParameterValue("stutterDiv");
        const float accent=std::pow(velocity,1.5f);
        // Lorenz Live
        cachedLorenzSigma=*tree.getRawParameterValue("lorenzSigma");
        cachedLorenzRho=*tree.getRawParameterValue("lorenzRho");
        cachedLorenzBeta=*tree.getRawParameterValue("lorenzBeta");
        cachedLorenzDt=*tree.getRawParameterValue("lorenzDt");
        // Weierstrass
        cachedWeiDim=*tree.getRawParameterValue("weiDimension");
        static const int weiLacTable[4]={2,3,5,7};
        cachedWeiLac=weiLacTable[juce::jlimit(0,3,(int)*tree.getRawParameterValue("weiLacunarity"))];
        cachedWeiDepth=2+juce::jlimit(0,10,(int)*tree.getRawParameterValue("weiDepth"));
        // Gray-Scott
        const bool  gsOn=*tree.getRawParameterValue("gsOn")>0.5f;
        const float gsFeed=*tree.getRawParameterValue("gsFeed");
        const float gsKill=*tree.getRawParameterValue("gsKill");
        const float gsDa=*tree.getRawParameterValue("gsDa");
        const float gsDb=*tree.getRawParameterValue("gsDb");
        const float gsVol=*tree.getRawParameterValue("gsVol");
        // Poly-temporale LFO Multiplikatoren
        static const float multTab[6]={1.f,1.6180339887f,3.1415926536f,1.4142135624f,1.7320508076f,2.7182818285f};
        const float lfoMult=multTab[juce::jlimit(0,5,(int)*tree.getRawParameterValue("lfoRateMult"))];
        const float lfo2Mult=multTab[juce::jlimit(0,5,(int)*tree.getRawParameterValue("lfo2RateMult"))];

        const float pbMult=std::pow(2.f,pitchBendSemitones/12.f);
        const float uniMult=std::pow(2.f,unisonDetuneCents/1200.f);
        const float osc1OctM=std::pow(2.f,(float)osc1Oct);
        const float osc2OctM=std::pow(2.f,(float)osc2Oct);
        const float osc2DetM=std::pow(2.f,osc2Det/1200.f);
        const double portaCoeff=porta<0.001f?1.0:juce::jlimit(0.0001,0.9999,1.0-std::exp(-1.0/(double(porta)*getSampleRate())));

        float stutterHz=0.f;
        if(stutterA>0.01f&&bpm>0.f){
            const float dt[]={1.f,0.5f,0.25f,0.125f,0.0625f,0.03125f,2.f/3.f,1.f/3.f,1.f/6.f,4.f/5.f,2.f/5.f,4.f/7.f,2.f/7.f};
            float div=(stuttDiv>=0&&stuttDiv<13)?dt[stuttDiv]:0.25f;
            stutterHz=bpm/(60.f*div);
        }
        if(osc1FOn){if(osc1FHP)miniFilter1.setHP(osc1FC);else miniFilter1.setLP(osc1FC);}
        miniFilter1.setBypassed(!osc1FOn);
        if(osc2FOn){if(osc2FHP)miniFilter2.setHP(osc2FC);else miniFilter2.setLP(osc2FC);}
        miniFilter2.setBypassed(!osc2FOn);
        miniFilterS1.setBypassed(!s1FOn);if(s1FOn)miniFilterS1.setLP(s1FC);
        miniFilterS2.setBypassed(!s2FOn);if(s2FOn)miniFilterS2.setLP(s2FC);
        filter.setType(static_cast<LadderFilter::Type>(ftype));
        const float panL=std::cos((stereoPan+1.f)*0.25f*juce::MathConstants<float>::pi);
        const float panR=std::sin((stereoPan+1.f)*0.25f*juce::MathConstants<float>::pi);
        const bool stereo=out.getNumChannels()>=2;
        const double sr=getSampleRate();
        auto loopMode=static_cast<VoiceSamplePlayer::LoopMode>(smpLoop);

        for(int i=start;i<start+num;++i)
        {
            currentFreq+=(targetFreq*pbMult*uniMult*osc1OctM-currentFreq)*portaCoeff;
            float lfo2Val=0.f;
            if(lfo2On){
                lfo2Phase+=(lfo2Rate*lfo2Mult)/sr;if(lfo2Phase>=1.0)lfo2Phase-=1.0;
                lfo2Val=(float)std::sin(lfo2Phase*juce::MathConstants<double>::twoPi)*lfo2Depth*lfo2Env.getNextSample();
            } else{lfo2Env.getNextSample();}
            float lfoRateActual=lfoRate*lfoMult;
            if(lfo2On&&lfo2Tgt==4)lfoRateActual=juce::jmax(0.01f,lfoRateActual*(1.f+lfo2Val*8.f));
            lfoPhase+=lfoRateActual/sr;if(lfoPhase>=1.0)lfoPhase-=1.0;
            const float lfo=(float)std::sin(lfoPhase*juce::MathConstants<double>::twoPi);
            float morphPos=morphPos0;
            if(morphAuto>0.001f){morphAutoPhase+=(0.05+morphAuto*0.4)/sr;if(morphAutoPhase>=1.0)morphAutoPhase-=1.0;morphPos=juce::jlimit(0.f,5.f,morphPos+(float)std::sin(morphAutoPhase*juce::MathConstants<double>::twoPi)*morphAuto*2.5f);}
            if(lfo2Tgt==3)morphPos=juce::jlimit(0.f,5.f,morphPos+lfo2Val*2.5f);
            shimmerPhase+=(currentFreq*1.0059)/sr;if(shimmerPhase>=1.0)shimmerPhase-=1.0;
            const float chaosPitch=1.f+chaos*0.02f*(float)std::sin(shimmerPhase*juce::MathConstants<double>::twoPi);
            const float pitchMod=(lfo2Tgt==0)?std::pow(2.f,lfo2Val*100.f/1200.f):1.f;
            double phaseInc=(currentFreq*chaosPitch*pitchMod)/sr;
            if(phaseDest>0.01f){phaseDestPhase+=(currentFreq*0.007*(1.0+phaseDest*3.0))/sr;if(phaseDestPhase>=1.0)phaseDestPhase-=1.0;double pdMod=std::sin(phaseDestPhase*6.28318*2.3)*std::cos(phaseDestPhase*6.28318*3.7);phaseInc=phaseInc*(1.0+phaseDest*pdMod*0.4)+juce::jlimit(-0.3,0.3,(double)pdFeedback*phaseDest*0.4)*phaseInc;}
            phase+=phaseInc;if(phase>=1.0)phase-=1.0;if(phase<0.0)phase+=1.0;
            const double osc2Freq=currentFreq*osc2OctM*osc2DetM/osc1OctM;
            osc2Phase+=osc2Freq/sr;if(osc2Phase>=1.0)osc2Phase-=1.0;
            driftPhase1+=0.0003f;driftPhase2+=0.0007f;
            const float pwmMod=(lfo2Tgt==2)?juce::jlimit(0.05f,0.95f,pwm+lfo2Val*0.45f):pwm;

            float o1L=0.f,o1R=0.f;
            if(osc1On){
                float o1=computeOsc(w1,phase,phaseInc,pwmMod,driftPhase1,driftPhase2,chaos,grainPhase,grainPos,sr,lorenzLiveX,lorenzLiveY,lorenzLiveZ);
                if((morphPos>0.05f||morphAuto>0.001f)&&w1<6){float mOsc=waveEngine.getMorphSample(phase,morphPos);float mMix=juce::jlimit(0.f,1.f,morphAuto*0.5f+(morphPos>0.1f?0.35f:0.f));o1=o1*(1.f-mMix)+mOsc*mMix;}
                pdFeedback=o1*0.3f;o1L=miniFilter1.processL(o1*osc1Vol);o1R=miniFilter1.processR(o1*osc1Vol);
            }
            float o2L=0.f,o2R=0.f;
            if(osc2On){float o2=computeOsc(w2,osc2Phase,osc2Freq/sr,osc2Pwm,driftPhase1+1.5f,driftPhase2+2.3f,chaos,osc2GranPhase,osc2GranPos,sr,lorenzLive2X,lorenzLive2Y,lorenzLive2Z);o2L=miniFilter2.processL(o2*osc2Vol);o2R=miniFilter2.processR(o2*osc2Vol);}
            float s1L=0.f,s1R=0.f;
            if(osc3On&&samplePlayer.hasFile()){if(!samplePlayer.isActive())samplePlayer.noteOn(midiNote,velocity);samplePlayer.getNextSample(s1L,s1R,osc3Vol,smpStart,smpEnd,effectiveSmpRoot,loopMode);s1L=miniFilterS1.processL(s1L);s1R=miniFilterS1.processR(s1R);}
            float s2L=0.f,s2R=0.f;
            if(osc4On&&grainPlayer.hasFile()){grainPlayer.baseRate=s2GrRate;grainPlayer.stutterAcc=s2StAcc;grainPlayer.startPt=s2Start;grainPlayer.endPt=s2End;grainPlayer.rootNote=effectiveSmp2Root;grainPlayer.volume=osc4Vol;grainPlayer.reverseMode=smp2Rev;if(!grainPlayer.isActive())grainPlayer.noteOn(midiNote,velocity);grainPlayer.getNextSample(s2L,s2R);s2L=miniFilterS2.processL(s2L);s2R=miniFilterS2.processR(s2R);}
            float granOut=0.f;
            if(gran1Mix>0.001f){gran1Phase+=(6.f+chaos*10.f)/(float)sr;if(gran1Phase>=1.f){gran1Phase-=1.f;gran1Pos=std::fmod(gran1Pos+0.07f+juce::Random::getSystemRandom().nextFloat()*0.15f,1.f);}float ge1=0.5f-0.5f*std::cos(gran1Phase*juce::MathConstants<float>::twoPi);float lp1=gran1Pos+gran1Phase*0.04f;lp1-=std::floor(lp1);granOut+=waveEngine.getSample(WaveEngine::WaveType::Granular,lp1)*ge1*gran1Mix*2.5f;}
            if(gran2Mix>0.001f){gran2Phase+=(18.f+chaos*30.f)/(float)sr;if(gran2Phase>=1.f){gran2Phase-=1.f;gran2Pos=std::fmod(gran2Pos+0.03f+juce::Random::getSystemRandom().nextFloat()*0.25f,1.f);}float ge2=0.5f-0.5f*std::cos(gran2Phase*juce::MathConstants<float>::twoPi);float lp2=gran2Pos+gran2Phase*0.08f;lp2-=std::floor(lp2);granOut+=waveEngine.getSample(WaveEngine::WaveType::FMFeedback,lp2)*ge2*gran2Mix*2.5f;}
            float gsOut=0.f;
            if(gsOn){if(!gsOsc.isActive())gsOsc.noteOn(midiNote,velocity);gsOsc.feed=gsFeed;gsOsc.kill=gsKill;gsOsc.Da=gsDa;gsOsc.Db=gsDb;gsOsc.volume=gsVol;gsOut=gsOsc.getNextSample();}
            float monoWave=o1L+o2L+granOut+gsOut;
            float monoSmpL=(s1L+s2L)*2.2f,monoSmpR=(s1R+s2R)*2.2f;
            float sigL,sigR;
            if(filterOn){const float fEnv=filterEnv.getNextSample();const float lfo2Cut=(lfo2Tgt==1)?lfo2Val*5000.f:0.f;const float fc=juce::jlimit(30.f,17000.f,cutoff+fEnv*envAmt*(6000.f+accent*4000.f)+lfo*lfoDepth*3000.f+lfo2Cut);filter.setParameters(fc,resonance);float fw=filter.process(monoWave);sigL=fw+monoSmpL;sigR=fw+monoSmpR;}
            else{filterEnv.getNextSample();sigL=monoWave+monoSmpL;sigR=monoWave+monoSmpR;}
            if(fxOn){
                if(drive>0.01f){sigL=std::tanh(sigL*(1.f+drive*4.f))/(1.f+drive*0.5f);sigR=std::tanh(sigR*(1.f+drive*4.f))/(1.f+drive*0.5f);}
                if(chaos2>0.01f){
                    auto fraktal=[&](float x,float phOff)->float{
                        const float r=1.6f+chaos2*2.4f;const int depth=1+(int)(chaos2*6.f);
                        float u=x/(1.f+r*0.12f);u=u*0.5f+0.5f;u=juce::jlimit(0.001f,0.999f,u);
                        for(int k=0;k<depth;++k)u=r*u*(1.f-u);
                        if(chaos2>0.45f){const float fA=(chaos2-0.45f)*(1.f/0.55f);float f1=std::fabs(std::fmod(u*(2.f+fA*6.f),2.f)-1.f);float f2=std::fabs(std::fmod(f1*(2.f+fA*4.f),2.f)-1.f);u=u*(1.f-fA*0.7f)+(f1*0.5f+f2*0.5f)*fA*0.7f;}
                        u+=std::sin(phOff+u*chaos2*12.f)*chaos2*0.055f;u=juce::jlimit(0.f,1.f,u);
                        return x*(1.f-chaos2)+(u*2.f-1.f)*chaos2;
                    };
                    const float phOff=(float)(phase*juce::MathConstants<double>::twoPi);
                    sigL=fraktal(sigL,phOff);sigR=fraktal(sigR,phOff+1.3f);
                }
                sigL=std::tanh(sigL*(1.f+resonance*1.5f));sigR=std::tanh(sigR*(1.f+resonance*1.5f));
            }
            if(timeFold>0.01f){timeFoldBuf[timeFoldPos%TIME_FOLD_SIZE]=(sigL+sigR)*0.5f;float tfp=(float)(timeFoldPos%TIME_FOLD_SIZE)/(float)TIME_FOLD_SIZE;float fp=tfp+timeFold*0.3f*std::sin(tfp*juce::MathConstants<float>::twoPi*2.f);fp-=std::floor(fp);int rp=(int)(fp*TIME_FOLD_SIZE)%TIME_FOLD_SIZE;float tfv=timeFoldBuf[rp];sigL=sigL*(1.f-timeFold*0.4f)+tfv*timeFold*0.4f;sigR=sigR*(1.f-timeFold*0.4f)+tfv*timeFold*0.4f;}
            ++timeFoldPos;if(timeFoldPos>=TIME_FOLD_SIZE*16)timeFoldPos=0;
            const float amp=ampEnv.getNextSample()*(0.3f+accent*0.5f);sigL*=amp;sigR*=amp;
            if(chaos>0.01f){flangerPhase+=0.25/sr;if(flangerPhase>=1.0)flangerPhase-=1.0;float fm=(float)std::sin(flangerPhase*juce::MathConstants<double>::twoPi);int dIdx=(flangerWrite+300+(int)(fm*180.f))&(FLANGER_SIZE-1);sigL+=flangerBuf[dIdx]*chaos*0.7f;sigR+=flangerBuf[dIdx]*chaos*0.7f;flangerBuf[flangerWrite&(FLANGER_SIZE-1)]=(sigL+sigR)*0.5f;sigL=std::tanh(sigL);sigR=std::tanh(sigR);}
            ++flangerWrite;
            if(stutterA>0.01f&&stutterHz>0.f){stutterPhase+=stutterHz/sr;if(stutterPhase>=1.0)stutterPhase-=1.0;float gate=(stutterPhase<(0.5f-stutterA*0.38f))?1.f:0.f;gate*=(0.7f+0.3f*(float)std::sin(stutterPhase*50.0));smoothedGate+=(gate-smoothedGate)*0.002f;sigL*=smoothedGate;sigR*=smoothedGate;}
            sigL=juce::jlimit(-1.f,1.f,sigL);sigR=juce::jlimit(-1.f,1.f,sigR);
            if(stereo){out.addSample(0,i,sigL*panL);out.addSample(1,i,sigR*panR);}
            else{out.addSample(0,i,(sigL+sigR)*0.5f);}
        }
        if(!ampEnv.isActive())clearCurrentNote();
    }

private:
    float computeOsc(int wi,double ph,double phInc,float pw,float dp1,float dp2,float chaos,float& gPh,float& gPos,double sr,float& lorX,float& lorY,float& lorZ)
    {
        if(wi==3)return WaveEngine::sawPolyBLEP(ph,phInc);
        if(wi==4)return WaveEngine::squarePWM(ph,phInc,pw);
        if(wi==5)return WaveEngine::driftSample(ph,dp1,dp2);
        if(wi==2){gPh+=(float)(10.0/sr);if(gPh>=1.f){gPh-=1.f;gPos=std::fmod(gPos+0.05f+juce::Random::getSystemRandom().nextFloat()*0.12f,1.f);}float ge=0.5f-0.5f*std::cos(gPh*juce::MathConstants<float>::twoPi);float lp=gPos+gPh*0.03f*(1.f+chaos*0.5f);lp-=std::floor(lp);return waveEngine.getSample(WaveEngine::WaveType::Granular,lp)*ge*3.f;}
        // Lorenz Live: Echtzeit-Attraktor, jede Voice hat eigenen Zustand
        if(wi==6){float dx=cachedLorenzSigma*(lorY-lorX);float dy=lorX*(cachedLorenzRho-lorZ)-lorY;float dz=lorX*lorY-cachedLorenzBeta*lorZ;lorX+=dx*cachedLorenzDt;lorY+=dy*cachedLorenzDt;lorZ+=dz*cachedLorenzDt;return juce::jlimit(-1.f,1.f,(lorX*0.6f+lorY*0.4f)/28.f);}
        // Weierstrass: fraktale additive Synthese, niemals differenzierbar
        if(wi==7){float b=(float)cachedWeiLac;float a=std::pow(b,-(2.f-cachedWeiDim));float t=(float)ph*juce::MathConstants<float>::twoPi;float res=0.f,scale=1.f,freq=1.f;for(int n=0;n<cachedWeiDepth;++n){res+=scale*std::cos(freq*t);scale*=a;freq*=b;}float norm=(a<0.9999f)?(1.f-a):0.1f;return juce::jlimit(-1.f,1.f,res*norm);}
        return waveEngine.getSample(static_cast<WaveEngine::WaveType>(juce::jlimit(0,5,wi)),ph);
    }

    const WaveEngine& waveEngine;
    juce::AudioProcessorValueTreeState& tree;
    VoiceSamplePlayer  samplePlayer;
    GrainStutterPlayer grainPlayer;
    GrayScottOscillator gsOsc;
    MiniFilter miniFilter1,miniFilter2,miniFilterS1,miniFilterS2;
    double currentFreq=0.0,targetFreq=440.0;
    double phase=0.0,osc2Phase=0.0;
    double lfoPhase=0.0,lfo2Phase=0.0,stutterPhase=0.0,flangerPhase=0.0;
    double shimmerPhase=0.0,morphAutoPhase=0.0,phaseDestPhase=0.0;
    float driftPhase1=0.f,driftPhase2=0.f;
    float velocity=1.f,smoothedGate=0.f,pdFeedback=0.f;
    float bpm=120.f,pitchBendSemitones=0.f,modWheelValue=0.f;
    float unisonDetuneCents=0.f,stereoPan=0.f;
    int midiNote=60;
    float grainPhase=0.f,grainPos=0.f;
    float gran1Phase=0.f,gran1Pos=0.f,gran2Phase=0.f,gran2Pos=0.f;
    float osc2GranPhase=0.f,osc2GranPos=0.f;
    float lorenzLiveX=0.1f,lorenzLiveY=0.f,lorenzLiveZ=20.f;
    float lorenzLive2X=0.2f,lorenzLive2Y=0.1f,lorenzLive2Z=25.f;
    float cachedLorenzSigma=10.f,cachedLorenzRho=28.f,cachedLorenzBeta=2.667f,cachedLorenzDt=0.002f;
    float cachedWeiDim=1.5f;int cachedWeiLac=3,cachedWeiDepth=6;
    juce::ADSR ampEnv,filterEnv,lfo2Env;
    LadderFilter filter;
    static constexpr int FLANGER_SIZE=2048,TIME_FOLD_SIZE=512;
    float flangerBuf[FLANGER_SIZE]={};int flangerWrite=0;
    std::array<float,TIME_FOLD_SIZE> timeFoldBuf{};int timeFoldPos=0;
};
