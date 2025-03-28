#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DrumMachineAudioProcessorEditor::DrumMachineAudioProcessorEditor (DrumMachineAudioProcessor& p)
    : AudioProcessorEditor (&p), 
      audioProcessor (p),
      tabbedComponent(juce::TabbedButtonBar::TabsAtTop),
      mainTab(),
      gameOfLifeTab(),
      gameOfLifeComponent(p.getParameterManager()),
      drumPadComponent(p.getParameterManager()),
      waveformVisualizer(2),
      noteActivityIndicator() // Added noteActivityIndicator member variable
{
    // Connect UI components to their models
    gameOfLifeComponent.setGameOfLife(&p.getGameOfLife());
    drumPadComponent.setDrumPads(p.drumPads.data());
    
    // Set up the tabbed component
    addAndMakeVisible(tabbedComponent);
    tabbedComponent.addTab("Main", juce::Colours::darkgrey, &mainTab, false);
    tabbedComponent.addTab("Game of Life", juce::Colours::darkgrey, &gameOfLifeTab, false);
    
    // Add components to the main tab
    mainTab.addAndMakeVisible(drumPadComponent);
    
    // Add components to the Game of Life tab
    gameOfLifeTab.addAndMakeVisible(gameOfLifeComponent);
    
    // Set up waveform visualizer (now directly added to the editor)
    waveformVisualizer.setBufferSize(256);
    waveformVisualizer.setSamplesPerBlock(16);
    waveformVisualizer.setColours(juce::Colours::black, juce::Colours::lightgreen);
    addAndMakeVisible(waveformVisualizer);
    
    // Set up note activity indicator
    addAndMakeVisible(noteActivityIndicator);
    
    // Set the plugin window size
    setSize(800, 800);
    
    // Start the timer for visualization updates
    startTimerHz(30);
}

DrumMachineAudioProcessorEditor::~DrumMachineAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void DrumMachineAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Fill the background
    g.fillAll(juce::Colours::darkgrey);
    
    // Draw title
    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawText("Synth of Life", getLocalBounds().removeFromTop(40), juce::Justification::centred, true);
}

void DrumMachineAudioProcessorEditor::resized()
{
    // Set up the layout
    auto area = getLocalBounds().reduced(20);
    
    // Reserve space for the title
    area.removeFromTop(40);
    
    // Position the tabbed component (leave space for waveform at bottom)
    auto tabArea = area.withHeight(area.getHeight() - 100);
    tabbedComponent.setBounds(tabArea);
    
    // Layout for the main tab
    auto mainArea = mainTab.getLocalBounds().reduced(10);
    
    // Position the drum pad component in the main tab
    drumPadComponent.setBounds(mainArea);
    
    // Layout for the Game of Life tab
    auto golArea = gameOfLifeTab.getLocalBounds().reduced(10);
    
    // Make the Game of Life component take up the entire space
    gameOfLifeComponent.setBounds(golArea);
    
    // Position the waveform visualizer and note activity indicator at the bottom (outside of tabs)
    auto bottomArea = area.removeFromBottom(80);
    
    // Position note activity indicator on the left
    auto indicatorArea = bottomArea.removeFromLeft(60);
    noteActivityIndicator.setBounds(indicatorArea);
    
    // Position waveform visualizer in the remaining space
    waveformVisualizer.setBounds(bottomArea);
}

void DrumMachineAudioProcessorEditor::timerCallback()
{
    // Update the waveform visualizer
    waveformVisualizer.pushBuffer(audioProcessor.getWaveformBuffer());
    
    // Update the note activity indicator
    noteActivityIndicator.setActive(audioProcessor.getNoteActiveStatus());
    
    // Trigger a repaint
    repaint();
}
