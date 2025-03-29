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
                                        public juce::Timer,
                                        public DrumMachineAudioProcessor::StateLoadedListener,
                                        public juce::TextEditor::Listener
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

    // Interval controls (moved from GameOfLifeComponent)
    juce::Label intervalLabel;
    juce::ComboBox intervalTypeBox;
    juce::ComboBox intervalValueBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> intervalTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> intervalValueAttachment;

    // Section iteration controls
    struct SectionControls {
        juce::Label titleLabel;
        juce::Label barsLabel;
        juce::Slider barsSlider;
        juce::Label gridStateLabel;
        juce::TextEditor gridStateTextBox;
        juce::ToggleButton randomizeToggle;
        juce::Label randomizeLabel;
        juce::Label densityLabel;
        juce::Slider densitySlider;
        
        // Parameter attachments
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> barsAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> randomizeAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> densityAttachment;
        
        // Current state tracking
        bool isActive = false;
        double remainingBars = 4.0;
    };
    
    SectionControls sections[4];
    int currentSection = 0;
    double beatsPerBar = 4.0;
    double lastBeatPosition = 0.0;
    
    // Section iteration methods
    void updateSectionIteration();
    void initializeGridForSection(int sectionIndex);
    void updateSectionUI();
    
    // Implement TextEditor::Listener
    void textEditorTextChanged(juce::TextEditor& textEditor) override {}
    void textEditorReturnKeyPressed(juce::TextEditor& textEditor) override;
    void textEditorEscapeKeyPressed(juce::TextEditor& textEditor) override {}
    void textEditorFocusLost(juce::TextEditor& textEditor) override;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumMachineAudioProcessorEditor)
};
