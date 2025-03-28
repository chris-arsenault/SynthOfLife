#pragma once

#include <JuceHeader.h>

/**
 * A simple component that displays a visual indicator when a note is active
 */
class NoteActivityIndicator : public juce::Component
{
public:
    NoteActivityIndicator();
    ~NoteActivityIndicator() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Set the active state of the indicator
    void setActive(bool isActive);
    
private:
    bool active = false;
    juce::Colour activeColour = juce::Colours::lightgreen;
    juce::Colour inactiveColour = juce::Colours::darkgrey;
};
