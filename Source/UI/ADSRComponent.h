#pragma once

#include <JuceHeader.h>

class ADSRComponent : public juce::Component,
                      private juce::Slider::Listener,
                      private juce::Label::Listener
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        
        virtual void attackChanged(float newValue) = 0;
        virtual void decayChanged(float newValue) = 0;
        virtual void sustainChanged(float newValue) = 0;
        virtual void releaseChanged(float newValue) = 0;
    };
    
    ADSRComponent()
    {
        // Create knobs for ADSR controls
        addAndMakeVisible(attackSlider);
        attackSlider.setRange(0.1, 2000.0, 0.1);
        attackSlider.setSkewFactorFromMidPoint(100.0);
        attackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        attackSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        attackSlider.setValue(10.0, juce::dontSendNotification);
        attackSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::orange);
        attackSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        attackSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::darkgrey);
        attackSlider.addListener(this);
        
        addAndMakeVisible(decaySlider);
        decaySlider.setRange(0.1, 2000.0, 0.1);
        decaySlider.setSkewFactorFromMidPoint(100.0);
        decaySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        decaySlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        decaySlider.setValue(100.0, juce::dontSendNotification);
        decaySlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::orange);
        decaySlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        decaySlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::darkgrey);
        decaySlider.addListener(this);
        
        addAndMakeVisible(sustainSlider);
        sustainSlider.setRange(0.0, 1.0, 0.01);
        sustainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        sustainSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        sustainSlider.setValue(0.7, juce::dontSendNotification);
        sustainSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::orange);
        sustainSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        sustainSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::darkgrey);
        sustainSlider.addListener(this);
        
        addAndMakeVisible(releaseSlider);
        releaseSlider.setRange(0.1, 5000.0, 0.1);
        releaseSlider.setSkewFactorFromMidPoint(500.0);
        releaseSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        releaseSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        releaseSlider.setValue(200.0, juce::dontSendNotification);
        releaseSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::orange);
        releaseSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        releaseSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::darkgrey);
        releaseSlider.addListener(this);
        
        // Create labels for sliders
        addAndMakeVisible(attackLabel);
        attackLabel.setText("Attack", juce::dontSendNotification);
        attackLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(decayLabel);
        decayLabel.setText("Decay", juce::dontSendNotification);
        decayLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(sustainLabel);
        sustainLabel.setText("Sustain", juce::dontSendNotification);
        sustainLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(releaseLabel);
        releaseLabel.setText("Release", juce::dontSendNotification);
        releaseLabel.setJustificationType(juce::Justification::centred);
        
        // Create title label
        addAndMakeVisible(titleLabel);
        titleLabel.setText("Envelope", juce::dontSendNotification);
        titleLabel.setFont(juce::Font(16.0f, juce::Font::bold));
        titleLabel.setJustificationType(juce::Justification::centred);
    }
    
    ~ADSRComponent() override
    {
        attackSlider.removeListener(this);
        decaySlider.removeListener(this);
        sustainSlider.removeListener(this);
        releaseSlider.removeListener(this);
    }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
        
        // Draw envelope curve
        g.setColour(juce::Colours::white);
        g.drawRect(envelopeCurveArea.toFloat());
        
        if (envelopeCurveArea.getWidth() > 0 && envelopeCurveArea.getHeight() > 0)
        {
            drawEnvelopeCurve(g);
        }
    }
    
    void resized() override
    {
        auto area = getLocalBounds();
        
        // Position title label
        titleLabel.setBounds(area.removeFromTop(25));
        
        // Create area for envelope curve visualization
        envelopeCurveArea = area.removeFromTop(80).reduced(5);
        
        // Remove some padding
        area.removeFromTop(10);
        
        // Calculate sizes for the knobs
        const int knobSize = 80;
        const int knobSpacing = 10;
        const int labelHeight = 20;
        
        // Create a horizontal layout for the knobs
        auto knobArea = area.reduced(10, 0);
        
        // Divide the area horizontally for the 4 knobs
        int knobWidth = (knobArea.getWidth() - (3 * knobSpacing)) / 4;
        
        // Position attack knob and label
        auto attackArea = knobArea.removeFromLeft(knobWidth);
        attackSlider.setBounds(attackArea.withHeight(knobSize));
        attackLabel.setBounds(attackArea.getX(), attackArea.getY() + knobSize + 5, knobWidth, labelHeight);
        
        knobArea.removeFromLeft(knobSpacing);
        
        // Position decay knob and label
        auto decayArea = knobArea.removeFromLeft(knobWidth);
        decaySlider.setBounds(decayArea.withHeight(knobSize));
        decayLabel.setBounds(decayArea.getX(), decayArea.getY() + knobSize + 5, knobWidth, labelHeight);
        
        knobArea.removeFromLeft(knobSpacing);
        
        // Position sustain knob and label
        auto sustainArea = knobArea.removeFromLeft(knobWidth);
        sustainSlider.setBounds(sustainArea.withHeight(knobSize));
        sustainLabel.setBounds(sustainArea.getX(), sustainArea.getY() + knobSize + 5, knobWidth, labelHeight);
        
        knobArea.removeFromLeft(knobSpacing);
        
        // Position release knob and label
        auto releaseArea = knobArea;
        releaseSlider.setBounds(releaseArea.withHeight(knobSize));
        releaseLabel.setBounds(releaseArea.getX(), releaseArea.getY() + knobSize + 5, knobWidth, labelHeight);
    }
    
    void setValues(float attack, float decay, float sustain, float release)
    {
        attackSlider.setValue(attack, juce::dontSendNotification);
        decaySlider.setValue(decay, juce::dontSendNotification);
        sustainSlider.setValue(sustain, juce::dontSendNotification);
        releaseSlider.setValue(release, juce::dontSendNotification);
        repaint();
    }
    
    void addListener(Listener* listener)
    {
        listeners.add(listener);
    }
    
    void removeListener(Listener* listener)
    {
        listeners.remove(listener);
    }
    
    // Method to connect the ADSR sliders to parameters
    void connectToParameters(juce::AudioProcessorValueTreeState& apvts, int sampleIndex)
    {
        // Create parameter attachments
        attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "attack_" + juce::String(sampleIndex), attackSlider);
            
        decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "decay_" + juce::String(sampleIndex), decaySlider);
            
        sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "sustain_" + juce::String(sampleIndex), sustainSlider);
            
        releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "release_" + juce::String(sampleIndex), releaseSlider);
            
        // Debug output
        DBG("Connected ADSR sliders to parameters for sample " + juce::String(sampleIndex));
    }
    
private:
    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;
    
    juce::Label attackLabel;
    juce::Label decayLabel;
    juce::Label sustainLabel;
    juce::Label releaseLabel;
    juce::Label titleLabel;
    
    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    
    juce::Rectangle<int> envelopeCurveArea;
    juce::ListenerList<Listener> listeners;
    
    void sliderValueChanged(juce::Slider* slider) override
    {
        if (slider == &attackSlider)
        {
            listeners.call([this](Listener& l) { l.attackChanged(static_cast<float>(attackSlider.getValue())); });
        }
        else if (slider == &decaySlider)
        {
            listeners.call([this](Listener& l) { l.decayChanged(static_cast<float>(decaySlider.getValue())); });
        }
        else if (slider == &sustainSlider)
        {
            listeners.call([this](Listener& l) { l.sustainChanged(static_cast<float>(sustainSlider.getValue())); });
        }
        else if (slider == &releaseSlider)
        {
            listeners.call([this](Listener& l) { l.releaseChanged(static_cast<float>(releaseSlider.getValue())); });
        }
        
        repaint();
    }
    
    void labelTextChanged(juce::Label* label) override
    {
        // Not used
    }
    
    void drawEnvelopeCurve(juce::Graphics& g)
    {
        float attack = static_cast<float>(attackSlider.getValue());
        float decay = static_cast<float>(decaySlider.getValue());
        float sustain = static_cast<float>(sustainSlider.getValue());
        float release = static_cast<float>(releaseSlider.getValue());
        
        // Normalize values to fit in the drawing area
        float totalTime = attack + decay + 500.0f + release; // Add some sustain time for visualization
        
        float attackWidth = (attack / totalTime) * envelopeCurveArea.getWidth();
        float decayWidth = (decay / totalTime) * envelopeCurveArea.getWidth();
        float sustainWidth = (500.0f / totalTime) * envelopeCurveArea.getWidth(); // Fixed sustain width for visualization
        float releaseWidth = (release / totalTime) * envelopeCurveArea.getWidth();
        
        float height = static_cast<float>(envelopeCurveArea.getHeight());
        
        // Create path for envelope curve
        juce::Path path;
        
        // Start at bottom left (zero amplitude)
        path.startNewSubPath(envelopeCurveArea.getX(), envelopeCurveArea.getBottom());
        
        // Attack phase (ramp up to max amplitude)
        path.lineTo(envelopeCurveArea.getX() + attackWidth, envelopeCurveArea.getY());
        
        // Decay phase (ramp down to sustain level)
        path.lineTo(envelopeCurveArea.getX() + attackWidth + decayWidth, 
                   envelopeCurveArea.getY() + (1.0f - sustain) * height);
        
        // Sustain phase (constant at sustain level)
        path.lineTo(envelopeCurveArea.getX() + attackWidth + decayWidth + sustainWidth, 
                   envelopeCurveArea.getY() + (1.0f - sustain) * height);
        
        // Release phase (ramp down to zero)
        path.lineTo(envelopeCurveArea.getX() + attackWidth + decayWidth + sustainWidth + releaseWidth, 
                   envelopeCurveArea.getBottom());
        
        // Draw the path
        g.setColour(juce::Colours::orange);
        g.strokePath(path, juce::PathStrokeType(2.0f));
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ADSRComponent)
};
