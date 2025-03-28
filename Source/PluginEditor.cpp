#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DrumMachineAudioProcessorEditor::DrumMachineAudioProcessorEditor (DrumMachineAudioProcessor& p)
    : AudioProcessorEditor (&p), 
      audioProcessor (p),
      tabbedComponent(juce::TabbedButtonBar::TabsAtTop),
      mainTab(),
      drumPadTab(),
      gameOfLifeTab(),
      sampleSettingsTab1(),
      sampleSettingsTab2(),
      sampleSettingsTab3(),
      sampleSettingsTab4(),
      gameOfLifeComponent(p.getParameterManager()),
      drumPadComponent(p.getParameterManager()),
      sampleSettings1(p.getParameterManager(), 0, 4),
      sampleSettings2(p.getParameterManager(), 4, 4),
      sampleSettings3(p.getParameterManager(), 8, 4),
      sampleSettings4(p.getParameterManager(), 12, 4),
      waveformVisualizer(2),
      noteActivityIndicator(),
      scaleSelector(),
      scaleSelectorLabel()
{
    // Connect UI components to their models
    gameOfLifeComponent.setGameOfLife(&p.getGameOfLife());
    drumPadComponent.setDrumPads(p.drumPads.data());
    
    // Connect drum pads to sample settings components
    sampleSettings1.setDrumPads(p.drumPads.data());
    sampleSettings2.setDrumPads(p.drumPads.data());
    sampleSettings3.setDrumPads(p.drumPads.data());
    sampleSettings4.setDrumPads(p.drumPads.data());
    
    // Set up the scale selector
    addAndMakeVisible(scaleSelector);
    addAndMakeVisible(scaleSelectorLabel);
    
    scaleSelectorLabel.setText("Musical Scale:", juce::dontSendNotification);
    scaleSelectorLabel.setJustificationType(juce::Justification::right);
    scaleSelectorLabel.attachToComponent(&scaleSelector, true);
    
    // Add scale options to the combobox
    scaleSelector.addItem("Major", 1);
    scaleSelector.addItem("Natural Minor", 2);
    scaleSelector.addItem("Harmonic Minor", 3);
    scaleSelector.addItem("Chromatic", 4);
    scaleSelector.addItem("Pentatonic", 5);
    scaleSelector.addItem("Blues", 6);
    
    // Create the attachment to link the combobox to the parameter
    scaleSelectorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        p.getParameterManager().getAPVTS(), "musicalScale", scaleSelector);
    
    // Set up the tabbed component
    addAndMakeVisible(tabbedComponent);
    tabbedComponent.addTab("Main", juce::Colours::darkgrey, &mainTab, false);
    tabbedComponent.addTab("Drum Pads", juce::Colours::darkgrey, &drumPadTab, false);
    tabbedComponent.addTab("Game of Life", juce::Colours::darkgrey, &gameOfLifeTab, false);
    
    // Add sample settings tabs
    tabbedComponent.addTab("Samples 1-4", juce::Colours::darkgrey, &sampleSettingsTab1, false);
    tabbedComponent.addTab("Samples 5-8", juce::Colours::darkgrey, &sampleSettingsTab2, false);
    tabbedComponent.addTab("Samples 9-12", juce::Colours::darkgrey, &sampleSettingsTab3, false);
    tabbedComponent.addTab("Samples 13-16", juce::Colours::darkgrey, &sampleSettingsTab4, false);
    
    // Add components to the main tab
    mainTab.addAndMakeVisible(scaleSelector);
    mainTab.addAndMakeVisible(scaleSelectorLabel);
    
    // Add components to the drum pad tab
    drumPadTab.addAndMakeVisible(drumPadComponent);
    
    // Add components to the Game of Life tab
    gameOfLifeTab.addAndMakeVisible(gameOfLifeComponent);
    
    // Add sample settings components to their respective tabs
    sampleSettingsTab1.addAndMakeVisible(sampleSettings1);
    sampleSettingsTab2.addAndMakeVisible(sampleSettings2);
    sampleSettingsTab3.addAndMakeVisible(sampleSettings3);
    sampleSettingsTab4.addAndMakeVisible(sampleSettings4);
    
    // Add waveform visualizer and note activity indicator to all tabs
    // These will be positioned at the bottom of each tab
    addAndMakeVisible(waveformVisualizer);
    addAndMakeVisible(noteActivityIndicator);
    
    // Set up the waveform visualizer
    waveformVisualizer.setColours(juce::Colours::black, juce::Colours::lightgreen);
    waveformVisualizer.setRepaintRate(30);
    waveformVisualizer.setBufferSize(512);
    
    // Set the audio processor to push samples to the visualizer
    p.setAudioVisualizer(&waveformVisualizer);
    
    // Set the audio processor to update the note activity indicator
    p.setNoteActivityIndicator(&noteActivityIndicator);
    
    // Register as a state loaded listener
    p.addStateLoadedListener(this);
    
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (800, 800);
    
    // Start the timer for UI updates
    startTimerHz(30);
}

DrumMachineAudioProcessorEditor::~DrumMachineAudioProcessorEditor()
{
    // Stop the timer
    stopTimer();
    
    // Disconnect the audio processor from the visualizer
    audioProcessor.setAudioVisualizer(nullptr);
    audioProcessor.setNoteActivityIndicator(nullptr);
    
    // Remove as state loaded listener
    audioProcessor.removeStateLoadedListener(this);
}

//==============================================================================
void DrumMachineAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Fill the background with a dark color
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void DrumMachineAudioProcessorEditor::resized()
{
    // Set the bounds of the tabbed component to fill most of the window
    auto area = getLocalBounds().reduced(4);
    
    // Reserve space at the bottom for the waveform visualizer and note activity indicator
    auto bottomArea = area.removeFromBottom(130);
    
    // Position the note activity indicator to the left of the waveform visualizer
    auto noteActivityArea = bottomArea.removeFromLeft(120);
    noteActivityArea = noteActivityArea.withTrimmedTop(30).withTrimmedBottom(30);
    noteActivityIndicator.setBounds(noteActivityArea);
    
    // Position the waveform visualizer to take up the rest of the bottom area
    waveformVisualizer.setBounds(bottomArea);
    
    // Set the bounds of the tabbed component to fill the remaining space
    tabbedComponent.setBounds(area);
    
    // Layout components within the main tab
    auto mainArea = mainTab.getLocalBounds().reduced(8);
    
    // Position the scale selector in the center of the main tab
    int labelWidth = 80;
    int selectorWidth = 220;
    int totalWidth = labelWidth + selectorWidth;
    int centerX = mainArea.getCentreX() - totalWidth / 2;
    int centerY = mainArea.getCentreY() - 15;
    
    scaleSelectorLabel.setBounds(centerX, centerY, labelWidth, 30);
    scaleSelector.setBounds(centerX + labelWidth, centerY, selectorWidth, 30);
    
    // Layout components within the drum pad tab
    drumPadComponent.setBounds(drumPadTab.getLocalBounds().reduced(8));
    
    // Layout components within the Game of Life tab
    gameOfLifeComponent.setBounds(gameOfLifeTab.getLocalBounds().reduced(8));
    
    // Layout components within the sample settings tabs
    sampleSettings1.setBounds(sampleSettingsTab1.getLocalBounds().reduced(8));
    sampleSettings2.setBounds(sampleSettingsTab2.getLocalBounds().reduced(8));
    sampleSettings3.setBounds(sampleSettingsTab3.getLocalBounds().reduced(8));
    sampleSettings4.setBounds(sampleSettingsTab4.getLocalBounds().reduced(8));
}

void DrumMachineAudioProcessorEditor::timerCallback()
{
    // Update any UI components that need regular refreshing
    gameOfLifeComponent.updateGrid();
    
    // Update the note activity indicator
    noteActivityIndicator.setActive(audioProcessor.isAnyNoteActive());
    
    // Update the waveform visualizer with the latest audio data from the processor
    waveformVisualizer.pushBuffer(audioProcessor.getVisualizationBuffer());
    
    // Repaint the UI
    repaint();
}

void DrumMachineAudioProcessorEditor::stateLoaded()
{
    // Update all UI components with the loaded state
    sampleSettings1.updateADSRComponentsFromDrumPads();
    sampleSettings2.updateADSRComponentsFromDrumPads();
    sampleSettings3.updateADSRComponentsFromDrumPads();
    sampleSettings4.updateADSRComponentsFromDrumPads();
    
    // Update other UI components as needed
    drumPadComponent.repaint();
}
