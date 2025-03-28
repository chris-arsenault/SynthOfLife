#pragma once

#include <JuceHeader.h>
#include "../ParameterManager.h"
#include "../DrumPad.h"
#include "ADSRComponent.h"

// A component that displays settings for a group of samples
class SampleSettingsComponent : public juce::Component,
                                public juce::Button::Listener,
                                public juce::FileDragAndDropTarget,
                                public ADSRComponent::Listener
{
public:
    SampleSettingsComponent(ParameterManager& paramManager, int startSampleIndex, int numSamples);
    ~SampleSettingsComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Button listener
    void buttonClicked(juce::Button* button) override;
    
    // File drag and drop
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    
    // Set the drum pads
    void setDrumPads(DrumPad* pads) 
    { 
        drumPads = pads; 
        updateADSRComponentsFromDrumPads();
    }
    
    // Load a sample for a pad
    void loadSampleForPad(int padIndex, const juce::File& file);
    
    // Update ADSR components from drum pads
    void updateADSRComponentsFromDrumPads();
    
private:
    ParameterManager& paramManager;
    int startSampleIndex;
    int numSamples;
    DrumPad* drumPads = nullptr;
    
    // File chooser for loading samples
    std::unique_ptr<juce::FileChooser> fileChooser;
    
    // UI components for each sample
    struct SampleControls
    {
        juce::Label nameLabel;
        juce::Slider volumeSlider;
        juce::Slider panSlider;
        juce::ToggleButton muteButton;
        juce::Slider midiNoteSlider;
        juce::Slider polyphonySlider;
        juce::ComboBox controlModeBox;
        juce::TextButton loadButton;
        juce::Label filenameLabel;
        juce::ToggleButton legatoButton;
        ADSRComponent adsrComponent;
        
        // Parameter attachments
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> muteAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> midiNoteAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> polyphonyAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> controlModeAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> legatoAttachment;
        
        // Delete copy constructor and assignment operator
        SampleControls() = default;
        SampleControls(const SampleControls&) = delete;
        SampleControls& operator=(const SampleControls&) = delete;
        // Allow move constructor and assignment
        SampleControls(SampleControls&&) = default;
        SampleControls& operator=(SampleControls&&) = default;
    };
    
    // Use OwnedArray instead of vector to manage memory automatically
    juce::OwnedArray<SampleControls> sampleControls;
    
    // ADSR envelope listener methods
    void attackChanged(float newValue) override;
    void decayChanged(float newValue) override;
    void sustainChanged(float newValue) override;
    void releaseChanged(float newValue) override;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleSettingsComponent)
};
