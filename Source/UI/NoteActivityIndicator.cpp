#include "NoteActivityIndicator.h"

NoteActivityIndicator::NoteActivityIndicator()
{
    setOpaque(false);
}

void NoteActivityIndicator::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    
    // Fill background
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Draw border
    g.setColour(juce::Colours::grey);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    
    // Draw indicator
    g.setColour(active ? activeColour : inactiveColour);
    
    // Create a smaller rectangle for the indicator
    auto indicatorBounds = bounds.reduced(4.0f);
    g.fillRoundedRectangle(indicatorBounds, 3.0f);
    
    // Add text
    g.setColour(juce::Colours::black);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("MIDI", indicatorBounds, juce::Justification::centred, true);
}

void NoteActivityIndicator::resized()
{
    // Nothing to do here
}

void NoteActivityIndicator::setActive(bool isActive)
{
    if (active != isActive)
    {
        active = isActive;
        repaint();
    }
}
