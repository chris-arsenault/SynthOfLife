#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/GameOfLifeComponent.h"
#include "UI/DrumPadComponent.h"
#include "UI/NoteActivityIndicator.h"
#include "UI/SampleSettingsComponent.h"

//==============================================================================
/**
*/
class DrumMachineAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer,
                                        public DrumMachineAudioProcessor::StateLoadedListener
{
public:
    DrumMachineAudioProcessorEditor(DrumMachineAudioProcessor&);
    ~DrumMachineAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    
    // StateLoadedListener implementation
    void stateLoaded() override;
    
private:
    // Timer callback for visualization updates
    void timerCallback() override;
    
    // Reference to the processor
    DrumMachineAudioProcessor& audioProcessor;
    
    // UI Components
    juce::TabbedComponent tabbedComponent{juce::TabbedButtonBar::TabsAtTop};
    juce::Component mainTab;
    juce::Component gameOfLifeTab;
    juce::Component drumPadTab;
    
    // Sample settings tabs (4 tabs, each with 4 samples)
    juce::Component sampleSettingsTab1;
    juce::Component sampleSettingsTab2;
    juce::Component sampleSettingsTab3;
    juce::Component sampleSettingsTab4;
    
    // UI Components
    GameOfLifeComponent gameOfLifeComponent;
    DrumPadComponent drumPadComponent;
    
    // Sample settings components (one for each tab)
    SampleSettingsComponent sampleSettings1;
    SampleSettingsComponent sampleSettings2;
    SampleSettingsComponent sampleSettings3;
    SampleSettingsComponent sampleSettings4;
    
    juce::AudioVisualiserComponent waveformVisualizer;
    
    // Note activity indicator
    NoteActivityIndicator noteActivityIndicator;
    
    // Scale selector
    juce::ComboBox scaleSelector;
    juce::Label scaleSelectorLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleSelectorAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumMachineAudioProcessorEditor)
};
