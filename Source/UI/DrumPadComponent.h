#pragma once

#include <JuceHeader.h>
#include "../DrumPad.h"
#include "../ParameterManager.h"

class DrumPadComponent : public juce::Component,
                         public juce::Button::Listener,
                         public juce::FileDragAndDropTarget
{
public:
    DrumPadComponent(ParameterManager& paramManager);
    ~DrumPadComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Button listener
    void buttonClicked(juce::Button* button) override;
    
    // File drag and drop
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    
    // Set the drum pads
    void setDrumPads(DrumPad* pads) { drumPads = pads; }
    
    // Load a sample for a pad
    void loadSampleForPad(int padIndex, const juce::File& file);
    
private:
    ParameterManager& paramManager;
    DrumPad* drumPads = nullptr;
    
    // File chooser for loading samples
    std::unique_ptr<juce::FileChooser> fileChooser;
    
    struct PadControls
    {
        std::unique_ptr<juce::TextButton> loadButton;
        std::unique_ptr<juce::Slider> volumeSlider;
        std::unique_ptr<juce::Slider> panSlider;
        std::unique_ptr<juce::ToggleButton> muteButton;
        std::unique_ptr<juce::Label> filenameLabel;
        std::unique_ptr<juce::Slider> polyphonySlider;
        std::unique_ptr<juce::Label> polyphonyLabel;
        
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> muteAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> polyphonyAttachment;
    };
    
    std::array<PadControls, ParameterManager::NUM_DRUM_PADS> padControls;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumPadComponent)
};
