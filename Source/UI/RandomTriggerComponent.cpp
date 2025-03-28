#include "RandomTriggerComponent.h"

RandomTriggerComponent::RandomTriggerComponent(ParameterManager& pm)
    : paramManager(pm)
{
    // Initialize enable button
    enableButton.setButtonText("Enable Random Trigger");
    enableButton.addListener(this);
    addAndMakeVisible(enableButton);
    
    // Initialize interval type combo box
    intervalTypeBox.addItem("Normal", 1);
    intervalTypeBox.addItem("Dotted", 2);
    intervalTypeBox.addItem("Triplet", 3);
    intervalTypeBox.setSelectedItemIndex(0);
    intervalTypeBox.addListener(this);
    addAndMakeVisible(intervalTypeBox);
    
    // Initialize interval value combo box
    intervalValueBox.addItem("1/4 (Quarter)", 1);
    intervalValueBox.addItem("1/8 (Eighth)", 2);
    intervalValueBox.addItem("1/16 (Sixteenth)", 3);
    intervalValueBox.setSelectedItemIndex(2); // Default to 1/16th notes
    intervalValueBox.addListener(this);
    addAndMakeVisible(intervalValueBox);
    
    // Create parameter attachments
    enableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        paramManager.getAPVTS(), "randomTriggerEnabled", enableButton);
        
    intervalTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        paramManager.getAPVTS(), "intervalType", intervalTypeBox);
        
    intervalValueAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        paramManager.getAPVTS(), "intervalValue", intervalValueBox);
}

RandomTriggerComponent::~RandomTriggerComponent()
{
    // Nothing to clean up
}

void RandomTriggerComponent::paint(juce::Graphics& g)
{
    // Fill background
    g.fillAll(juce::Colours::darkgrey);
    
    // Draw section title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.drawText("Random Trigger", getLocalBounds().removeFromTop(30), juce::Justification::centred);
    
    // Draw labels
    g.setFont(juce::Font(14.0f));
    g.drawText("Interval Type:", 10, 40, 120, 20, juce::Justification::centredLeft);
    g.drawText("Interval Value:", 10, 70, 120, 20, juce::Justification::centredLeft);
}

void RandomTriggerComponent::resized()
{
    auto area = getLocalBounds();
    
    // Layout for title
    area.removeFromTop(30);
    
    // Layout for enable button
    enableButton.setBounds(area.removeFromTop(30).withSizeKeepingCentre(250, 28));
    
    // Layout for interval type
    auto typeRow = area.removeFromTop(30);
    typeRow.removeFromLeft(130); // Space for label
    intervalTypeBox.setBounds(typeRow.withWidth(200).withHeight(28));
    
    // Layout for interval value
    auto valueRow = area.removeFromTop(30);
    valueRow.removeFromLeft(130); // Space for label
    intervalValueBox.setBounds(valueRow.withWidth(200).withHeight(28));
}

void RandomTriggerComponent::buttonClicked(juce::Button* button)
{
    // Handle button clicks if needed
}

void RandomTriggerComponent::comboBoxChanged(juce::ComboBox* comboBox)
{
    // Handle combo box changes if needed
}
