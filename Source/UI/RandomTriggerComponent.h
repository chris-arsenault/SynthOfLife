#pragma once

#include <JuceHeader.h>
#include "../ParameterManager.h"

class RandomTriggerComponent : public juce::Component,
                               public juce::Button::Listener,
                               public juce::ComboBox::Listener
{
public:
    RandomTriggerComponent(ParameterManager& paramManager);
    ~RandomTriggerComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Button listener
    void buttonClicked(juce::Button* button) override;
    
    // ComboBox listener
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    
private:
    ParameterManager& paramManager;
    
    juce::ToggleButton enableButton;
    juce::ComboBox intervalTypeBox;
    juce::ComboBox intervalValueBox;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> intervalTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> intervalValueAttachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RandomTriggerComponent)
};
