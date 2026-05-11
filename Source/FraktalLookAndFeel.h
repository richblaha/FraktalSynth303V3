#pragma once
#include <JuceHeader.h>
#include <cmath>

class FraktalLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FraktalLookAndFeel()
    {
        setColour(juce::PopupMenu::backgroundColourId,       juce::Colour(0xFF181818));
        setColour(juce::PopupMenu::textColourId,             juce::Colour(0xFFAAAAAA));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xFF2A2A2A));
        setColour(juce::ComboBox::backgroundColourId,        juce::Colour(0xFF141414));
        setColour(juce::ComboBox::outlineColourId,           juce::Colour(0xFF2A2A2A));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float>((float)x,(float)y,(float)width,(float)height).reduced(4.f);
        auto centre = bounds.getCentre();
        float radius = juce::jmin(bounds.getWidth(),bounds.getHeight())*0.5f;
        float trackW = juce::jmax(2.f,radius*0.15f);
        float arcR   = radius-trackW*0.6f;
        // Background track
        {juce::Path bg;bg.addArc(centre.x-arcR,centre.y-arcR,arcR*2.f,arcR*2.f,rotaryStartAngle,rotaryEndAngle,true);g.setColour(juce::Colour(0xFF252525));g.strokePath(bg,juce::PathStrokeType(trackW,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
        // Value arc
        float toAngle=rotaryStartAngle+sliderPos*(rotaryEndAngle-rotaryStartAngle);
        auto  ac=slider.findColour(juce::Slider::rotarySliderFillColourId);
        {juce::Path val;val.addArc(centre.x-arcR,centre.y-arcR,arcR*2.f,arcR*2.f,rotaryStartAngle,toAngle,true);g.setColour(ac);g.strokePath(val,juce::PathStrokeType(trackW,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
        // Inner fill
        float innerR=arcR-trackW*0.5f-1.f;
        g.setColour(juce::Colour(0xFF0E0E0E));
        g.fillEllipse(centre.x-innerR,centre.y-innerR,innerR*2.f,innerR*2.f);
        // Indicator line
        float lineLen=innerR*0.78f;
        float tipX=centre.x+std::sin(toAngle)*lineLen;
        float tipY=centre.y-std::cos(toAngle)*lineLen;
        g.setColour(ac);
        g.drawLine(centre.x,centre.y,tipX,tipY,1.3f);
        g.fillEllipse(tipX-1.8f,tipY-1.8f,3.6f,3.6f);
        // Tick marks: min / centre / max
        float midAngle=(rotaryStartAngle+rotaryEndAngle)*0.5f;
        auto drawTick=[&](float angle,float alpha){
            float outer=arcR+trackW*0.55f,inner2=arcR-trackW*0.6f;
            float sinA=std::sin(angle),cosA=std::cos(angle);
            g.setColour(juce::Colour(0xFF555555).withAlpha(alpha));
            g.drawLine(centre.x+sinA*inner2,centre.y-cosA*inner2,centre.x+sinA*outer,centre.y-cosA*outer,1.2f);
        };
        drawTick(rotaryStartAngle,0.85f);drawTick(rotaryEndAngle,0.85f);drawTick(midAngle,0.50f);
        // Value text in centre
        {
            double v=slider.getValue();juce::String txt;
            if(std::abs(v)<1.0)txt=juce::String(v,2);
            else if(std::abs(v)<10.0)txt=juce::String(v,1);
            else if(std::abs(v)<999.5)txt=juce::String((int)std::round(v));
            else txt=juce::String(v/1000.0,1)+"k";
            g.setFont(juce::FontOptions("Helvetica Neue",6.0f,juce::Font::plain));
            g.setColour(ac.withAlpha(0.72f));
            g.drawText(txt,(int)(centre.x-innerR+1),(int)(centre.y-5.f),(int)(innerR*2.f-2),10,juce::Justification::centred);
        }
    }
};
