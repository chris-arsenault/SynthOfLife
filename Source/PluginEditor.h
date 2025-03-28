#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/GameOfLifeComponent.h"
#include "UI/DrumPadComponent.h"
#include "UI/NoteActivityIndicator.h"

//==============================================================================
/**
*/
class DrumMachineAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    DrumMachineAudioProcessorEditor(DrumMachineAudioProcessor&);
    ~DrumMachineAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    
private:
    // Timer callback for visualization updates
    void timerCallback() override;
    
    // Reference to the processor
    DrumMachineAudioProcessor& audioProcessor;
    
    // UI Components
    juce::TabbedComponent tabbedComponent{juce::TabbedButtonBar::TabsAtTop};
    juce::Component mainTab;
    juce::Component gameOfLifeTab;
    GameOfLifeComponent gameOfLifeComponent;
    DrumPadComponent drumPadComponent;
    juce::AudioVisualiserComponent waveformVisualizer;
    NoteActivityIndicator noteActivityIndicator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumMachineAudioProcessorEditor)
};
