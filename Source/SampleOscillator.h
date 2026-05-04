#pragma once
#include <JuceHeader.h>

// ═══════════════════════════════════════════════════════════════════
//  SharedSampleBuffer  –  einmal laden, alle Voices lesen
// ═══════════════════════════════════════════════════════════════════
class SharedSampleBuffer
{
public:
    enum class LoopMode { Off=0, Forward=1, PingPong=2 };

    SharedSampleBuffer() { formatManager.registerBasicFormats(); }

    bool loadFile (const juce::File& file)
    {
        auto* reader = formatManager.createReaderFor (file);
        if (!reader) return false;
        juce::AudioBuffer<float> tmp;
        tmp.setSize ((int)reader->numChannels, (int)reader->lengthInSamples);
        reader->read (&tmp, 0, (int)reader->lengthInSamples, 0, true, true);
        delete reader;
        const juce::ScopedWriteLock sl (lock);
        buffer   = std::move (tmp);
        fileName = file.getFileName();
        filePath = file.getFullPathName();
        return true;
    }

    bool         hasFile()     const { return buffer.getNumSamples() > 0; }
    juce::String getFileName() const { return fileName; }
    juce::String getFilePath() const { return filePath; }
    int   numSamples()  const { return buffer.getNumSamples(); }
    int   numChannels() const { return buffer.getNumChannels(); }

    float getSample (int ch, int pos) const
    {
        if (buffer.getNumSamples()==0) return 0.f;
        pos = juce::jlimit(0, buffer.getNumSamples()-1, pos);
        ch  = juce::jlimit(0, buffer.getNumChannels()-1, ch);
        return buffer.getSample(ch, pos);
    }

private:
    juce::AudioFormatManager formatManager;
    juce::AudioBuffer<float> buffer;
    juce::ReadWriteLock      lock;
    juce::String             fileName, filePath;
};

// ═══════════════════════════════════════════════════════════════════
//  VoiceSamplePlayer  –  Standard Sample Player (OSC 3)
// ═══════════════════════════════════════════════════════════════════
class VoiceSamplePlayer
{
public:
    using LoopMode = SharedSampleBuffer::LoopMode;

    void setBuffer (const SharedSampleBuffer* buf) { sharedBuf=buf; }

    void noteOn (int midi, float vel)
    {
        midiNote=midi; velocity=vel; playhead=0.0;
        pingPongFwd=true; active=true;
    }
    void noteOff() {}
    void reset()   { active=false; playhead=0.0; }
    bool isActive() const { return active; }
    bool hasFile()  const { return sharedBuf&&sharedBuf->hasFile(); }

    void getNextSample (float& outL, float& outR,
                        float volume, float startPt, float endPt,
                        int rootNote, LoopMode loop)
    {
        outL=outR=0.f;
        if (!active||!sharedBuf||!sharedBuf->hasFile()) return;
        int   nSmp  = sharedBuf->numSamples();
        int   nCh   = sharedBuf->numChannels();
        float sStart= startPt*nSmp;
        float sEnd  = juce::jmax(sStart+1.f, endPt*nSmp);
        double ratio= std::pow(2.0,(midiNote-rootNote)/12.0);

        if (playhead>=sEnd||playhead<sStart)
        {
            switch(loop) {
                case LoopMode::Off:     active=false; return;
                case LoopMode::Forward: playhead=sStart; break;
                case LoopMode::PingPong:
                    pingPongFwd=!pingPongFwd;
                    playhead=pingPongFwd?sStart:sEnd-1.0; break;
            }
        }
        int ia=juce::jlimit(0,nSmp-1,(int)playhead);
        int ib=juce::jlimit(0,nSmp-1,ia+1);
        float fr=(float)(playhead-std::floor(playhead));
        outL=sharedBuf->getSample(0,ia)*(1.f-fr)+sharedBuf->getSample(0,ib)*fr;
        outR=(nCh>1)?sharedBuf->getSample(1,ia)*(1.f-fr)+sharedBuf->getSample(1,ib)*fr:outL;
        outL*=volume*velocity; outR*=volume*velocity;
        if(pingPongFwd) playhead+=ratio; else playhead-=ratio;
    }

private:
    const SharedSampleBuffer* sharedBuf=nullptr;
    double playhead=0.0;
    float  velocity=1.f;
    int    midiNote=60;
    bool   active=false, pingPongFwd=true;
};

// ═══════════════════════════════════════════════════════════════════
//  GrainStutterPlayer  –  wie VoiceSamplePlayer + Grain-Hüllkurve
// ═══════════════════════════════════════════════════════════════════
class GrainStutterPlayer
{
public:
    using LoopMode = SharedSampleBuffer::LoopMode;

    void setBuffer    (const SharedSampleBuffer* buf) { sharedBuf=buf; }
    void setSampleRate(double sr) { sampleRate=sr; }

    void noteOn (int midi, float vel)
    {
        midiNote=midi; velocity=vel;
        playhead=0.0; grainPhase=0.0;
        pingPongFwd=true; active=true;
    }
    void noteOff() {}
    void reset()   { active=false; }
    bool isActive() const { return active; }
    bool hasFile()  const { return sharedBuf && sharedBuf->hasFile(); }

    float baseRate   = 8.f;    // Grain-Rate Hz
    float stutterAcc = 0.f;    // Speed: -1=halbe, 0=normal, +1=doppelt
    float startPt    = 0.f;
    float endPt      = 1.f;
    float volume     = 0.7f;
    int   rootNote   = 60;

    void getNextSample (float& outL, float& outR)
    {
        outL=outR=0.f;
        if (!active||!sharedBuf||!sharedBuf->hasFile()) return;

        const int    nSmp   = sharedBuf->numSamples();
        const int    nCh    = sharedBuf->numChannels();
        const double sStart = startPt * nSmp;
        const double sEnd   = juce::jmax(sStart+2.0, endPt*(double)nSmp);

        // Playback — wie VoiceSamplePlayer, speed via stutterAcc
        const double ratio = std::pow(2.0,(midiNote-rootNote)/12.0);
        const double speed = ratio * (1.0 + (double)stutterAcc); // 0=half, 1=normal, 2=double

        if (pingPongFwd) {
            if (playhead >= sEnd) { pingPongFwd=false; playhead=sEnd-1; }
        } else {
            if (playhead <= sStart) { pingPongFwd=true; playhead=sStart; }
        }

        const int   ia = juce::jlimit(0,nSmp-1,(int)playhead);
        const int   ib = juce::jlimit(0,nSmp-1,ia+1);
        const float fr = (float)(playhead-std::floor(playhead));
        outL = sharedBuf->getSample(0,ia)*(1.f-fr)+sharedBuf->getSample(0,ib)*fr;
        outR = (nCh>1) ? sharedBuf->getSample(1,ia)*(1.f-fr)+sharedBuf->getSample(1,ib)*fr : outL;

        if (pingPongFwd) playhead+=speed; else playhead-=speed;

        // Grain-Hüllkurve (Trance-Gate)
        grainPhase += juce::jlimit(0.5f,64.f,baseRate) / sampleRate;
        if (grainPhase>=1.0) grainPhase-=1.0;
        const float env = 0.5f-0.5f*(float)std::cos(grainPhase*juce::MathConstants<double>::twoPi);

        outL *= volume * velocity * env;
        outR *= volume * velocity * env;
    }

private:
    const SharedSampleBuffer* sharedBuf=nullptr;
    double sampleRate=44100.0;
    double playhead=0.0, grainPhase=0.0;
    float  velocity=1.f;
    int    midiNote=60;
    bool   active=false, pingPongFwd=true;
};
