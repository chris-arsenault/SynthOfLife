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
      scaleSelectorLabel(),
      intervalLabel(),
      intervalTypeBox(),
      intervalValueBox(),
      currentSection(0),
      beatsPerBar(4.0),
      lastBeatPosition(0.0)
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
    scaleSelector.addItem("Chromatic", 1);
    scaleSelector.addItem("Major", 2);
    scaleSelector.addItem("Minor", 3);
    scaleSelector.addItem("Pentatonic", 4);
    scaleSelector.addItem("Blues", 5);
    scaleSelector.setSelectedId(4); // Default to Pentatonic
    
    // Initialize section controls
    for (int i = 0; i < 4; ++i)
    {
        // Set up title label
        sections[i].titleLabel.setText("Section " + juce::String(i + 1), juce::dontSendNotification);
        sections[i].titleLabel.setFont(juce::Font(16.0f, juce::Font::bold));
        sections[i].titleLabel.setColour(juce::Label::textColourId, i == 0 ? juce::Colours::green : juce::Colours::white);
        
        // Set up bars label and slider
        sections[i].barsLabel.setText("Bars:", juce::dontSendNotification);
        sections[i].barsLabel.setJustificationType(juce::Justification::right);
        
        sections[i].barsSlider.setRange(1.0, 16.0, 1.0);
        sections[i].barsSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        sections[i].barsSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        
        // Create parameter attachment for bars slider
        sections[i].barsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            p.getParameterManager().getAPVTS(), "section_bars_" + juce::String(i), sections[i].barsSlider);
        
        // Set up grid state label and text box
        sections[i].gridStateLabel.setText("Grid State:", juce::dontSendNotification);
        sections[i].gridStateLabel.setJustificationType(juce::Justification::right);
        
        sections[i].gridStateTextBox.setMultiLine(false);
        sections[i].gridStateTextBox.setReturnKeyStartsNewLine(false);
        sections[i].gridStateTextBox.setReadOnly(false);
        sections[i].gridStateTextBox.setScrollbarsShown(false);
        sections[i].gridStateTextBox.setCaretVisible(true);
        sections[i].gridStateTextBox.setPopupMenuEnabled(true);
        
        // Initialize with the parameter value
        auto* gridStateParam = p.getParameterManager().getSectionGridStateParam(i);
        if (gridStateParam != nullptr)
        {
            sections[i].gridStateTextBox.setText(juce::String(gridStateParam->get()), false);
        }
        else
        {
            sections[i].gridStateTextBox.setText("0", false);
        }
        
        // Add listener to update the parameter when text changes
        sections[i].gridStateTextBox.addListener(this);
        
        // Set up randomize toggle and label
        sections[i].randomizeToggle.setToggleState(true, juce::dontSendNotification);
        
        // Create parameter attachment for randomize toggle
        sections[i].randomizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            p.getParameterManager().getAPVTS(), "section_randomize_" + juce::String(i), sections[i].randomizeToggle);
        
        sections[i].randomizeLabel.setText("Randomize", juce::dontSendNotification);
        sections[i].randomizeLabel.setJustificationType(juce::Justification::left);
        
        // Set up density label and slider
        sections[i].densityLabel.setText("Density:", juce::dontSendNotification);
        sections[i].densityLabel.setJustificationType(juce::Justification::right);
        
        sections[i].densitySlider.setRange(0.1, 0.9, 0.01);
        sections[i].densitySlider.setSliderStyle(juce::Slider::LinearHorizontal);
        sections[i].densitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        
        // Create parameter attachment for density slider
        sections[i].densityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            p.getParameterManager().getAPVTS(), "section_density_" + juce::String(i), sections[i].densitySlider);
        
        // Initialize section state
        sections[i].isActive = (i == 0); // First section is active by default
        
        // Initialize remaining bars from the parameter
        auto* barsParam = p.getParameterManager().getSectionBarsParam(i);
        if (barsParam != nullptr)
        {
            sections[i].remainingBars = barsParam->get();
        }
        else
        {
            sections[i].remainingBars = 4.0;
        }
    }
    
    // Set up the interval controls
    addAndMakeVisible(intervalLabel);
    addAndMakeVisible(intervalTypeBox);
    addAndMakeVisible(intervalValueBox);
    
    intervalLabel.setText("Update Interval:", juce::dontSendNotification);
    intervalLabel.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 14.0f, juce::Font::bold));
    
    // Set up interval type combo box
    intervalTypeBox.addItem("Normal", 1);
    intervalTypeBox.addItem("Dotted", 2);
    intervalTypeBox.addItem("Triplet", 3);
    intervalTypeBox.setSelectedItemIndex(0);
    
    // Set up interval value combo box
    intervalValueBox.addItem("1/4 (Quarter)", 1);
    intervalValueBox.addItem("1/8 (Eighth)", 2);
    intervalValueBox.addItem("1/16 (Sixteenth)", 3);
    intervalValueBox.setSelectedItemIndex(2);
    
    // Create attachments for interval controls
    intervalTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        p.getParameterManager().getAPVTS(), "intervalType", intervalTypeBox);
    intervalValueAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        p.getParameterManager().getAPVTS(), "intervalValue", intervalValueBox);
    
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
    mainTab.addAndMakeVisible(intervalLabel);
    mainTab.addAndMakeVisible(intervalTypeBox);
    mainTab.addAndMakeVisible(intervalValueBox);
    
    // Add section iteration controls to the main tab
    for (int i = 0; i < 4; ++i)
    {
        mainTab.addAndMakeVisible(sections[i].titleLabel);
        mainTab.addAndMakeVisible(sections[i].barsLabel);
        mainTab.addAndMakeVisible(sections[i].barsSlider);
        mainTab.addAndMakeVisible(sections[i].gridStateLabel);
        mainTab.addAndMakeVisible(sections[i].gridStateTextBox);
        mainTab.addAndMakeVisible(sections[i].randomizeToggle);
        mainTab.addAndMakeVisible(sections[i].randomizeLabel);
        mainTab.addAndMakeVisible(sections[i].densityLabel);
        mainTab.addAndMakeVisible(sections[i].densitySlider);
    }
    
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
    auto area = getLocalBounds().reduced(10);
    
    // Position the tab component to take up most of the space
    auto tabArea = area.removeFromTop(area.getHeight() - 150);
    tabbedComponent.setBounds(tabArea);
    
    // Position the scale selector at the top of the main tab
    auto mainTabArea = tabbedComponent.getLocalBounds().reduced(10);
    auto scaleArea = mainTabArea.removeFromTop(30);
    
    scaleSelectorLabel.setBounds(scaleArea.removeFromLeft(120));
    scaleSelector.setBounds(scaleArea.removeFromLeft(200));
    
    // Position the interval controls below the scale selector
    auto intervalArea = mainTabArea.removeFromTop(30);
    intervalLabel.setBounds(intervalArea.removeFromLeft(120));
    intervalTypeBox.setBounds(intervalArea.removeFromLeft(150));
    intervalValueBox.setBounds(intervalArea.removeFromLeft(150));
    
    // Position the section iteration controls
    auto sectionsArea = mainTabArea.removeFromTop(240); // 60 height per section
    for (int i = 0; i < 4; ++i)
    {
        auto sectionArea = sectionsArea.removeFromTop(60);
        
        // Title label at the top
        sections[i].titleLabel.setBounds(sectionArea.removeFromTop(20));
        
        // Controls in the remaining area
        auto controlsArea = sectionArea.removeFromTop(30);
        
        // Bars label and slider
        sections[i].barsLabel.setBounds(controlsArea.removeFromLeft(50));
        sections[i].barsSlider.setBounds(controlsArea.removeFromLeft(150));
        
        // Grid state label and text box
        sections[i].gridStateLabel.setBounds(controlsArea.removeFromLeft(80));
        sections[i].gridStateTextBox.setBounds(controlsArea.removeFromLeft(150));
        
        // Randomize toggle and label
        sections[i].randomizeToggle.setBounds(controlsArea.removeFromLeft(30));
        sections[i].randomizeLabel.setBounds(controlsArea.removeFromLeft(100));
        
        // Density label and slider
        sections[i].densityLabel.setBounds(controlsArea.removeFromLeft(60));
        sections[i].densitySlider.setBounds(controlsArea.removeFromLeft(150));
    }
    
    // Position the drum pad component to fill the drum pad tab
    drumPadComponent.setBounds(drumPadTab.getLocalBounds().reduced(8));
    
    // Position the Game of Life component to fill the Game of Life tab
    gameOfLifeComponent.setBounds(gameOfLifeTab.getLocalBounds().reduced(8));
    
    // Position the sample settings components to fill their respective tabs
    sampleSettings1.setBounds(sampleSettingsTab1.getLocalBounds().reduced(8));
    sampleSettings2.setBounds(sampleSettingsTab2.getLocalBounds().reduced(8));
    sampleSettings3.setBounds(sampleSettingsTab3.getLocalBounds().reduced(8));
    sampleSettings4.setBounds(sampleSettingsTab4.getLocalBounds().reduced(8));
    
    // Position the waveform visualizer and note activity indicator at the bottom of the window
    auto bottomArea = area.removeFromBottom(130);
    auto noteActivityArea = bottomArea.removeFromLeft(120);
    noteActivityArea = noteActivityArea.withTrimmedTop(30).withTrimmedBottom(30);
    noteActivityIndicator.setBounds(noteActivityArea);
    waveformVisualizer.setBounds(bottomArea);
}

void DrumMachineAudioProcessorEditor::timerCallback()
{
    // Update any UI components that need regular refreshing
    gameOfLifeComponent.updateGrid();
    
    // Update the note activity indicator
    noteActivityIndicator.setActive(audioProcessor.isAnyNoteActive());
    
    // Update the waveform visualizer with the latest audio data from the processor
    waveformVisualizer.pushBuffer(audioProcessor.getVisualizationBuffer());
    
    // Update section iteration
    updateSectionIteration();
    
    // Repaint the UI
    repaint();
}

void DrumMachineAudioProcessorEditor::updateSectionIteration()
{
    // Get the playhead position from the audio processor
    auto* playHead = audioProcessor.getPlayHead();
    if (playHead == nullptr)
        return;
    
    juce::AudioPlayHead::CurrentPositionInfo posInfo;
    if (!playHead->getCurrentPosition(posInfo))
        return;
    
    // Calculate current beat position and check if transport is playing
    double currentBeatPosition = posInfo.ppqPosition;
    bool isPlaying = posInfo.isPlaying;
    
    // Get the time signature from the host
    int numerator = posInfo.timeSigNumerator;
    int denominator = posInfo.timeSigDenominator;
    
    // Calculate the number of beats per bar based on the time signature
    beatsPerBar = (numerator * 4.0) / denominator;
    
    // If transport is not playing, just update UI and return
    if (!isPlaying)
    {
        updateSectionUI();
        return;
    }
    
    // Calculate the total cycle length in bars
    double totalCycleLength = 0.0;
    for (int i = 0; i < 4; ++i)
    {
        auto* barsParam = audioProcessor.getParameterManager().getSectionBarsParam(i);
        if (barsParam != nullptr)
        {
            totalCycleLength += barsParam->get();
        }
        else
        {
            totalCycleLength += 4.0; // Default to 4 bars
        }
    }
    
    // Calculate position within the cycle (in bars)
    double positionInCycle = std::fmod(currentBeatPosition / beatsPerBar, totalCycleLength);
    if (positionInCycle < 0)
        positionInCycle += totalCycleLength;
    
    // Determine which section we're in based on position
    double sectionBoundary = 0.0;
    int newSection = 0;
    
    for (int i = 0; i < 4; ++i)
    {
        double sectionLength = 4.0; // Default
        auto* barsParam = audioProcessor.getParameterManager().getSectionBarsParam(i);
        if (barsParam != nullptr)
        {
            sectionLength = barsParam->get();
        }
        
        sectionBoundary += sectionLength;
        
        if (positionInCycle < sectionBoundary || i == 3) // Last section catches any remainder
        {
            newSection = i;
            
            // Calculate remaining bars in this section
            double remainingBars = sectionBoundary - positionInCycle;
            sections[i].remainingBars = remainingBars;
            
            break;
        }
    }
    
    // If the section has changed, update the grid
    if (newSection != currentSection)
    {
        // Deactivate old section
        sections[currentSection].isActive = false;
        
        // Activate new section
        currentSection = newSection;
        sections[currentSection].isActive = true;
        
        // Initialize the grid for the new section
        initializeGridForSection(currentSection);
    }
    
    // Update the last beat position
    lastBeatPosition = currentBeatPosition;
    
    // Update the UI
    updateSectionUI();
}

void DrumMachineAudioProcessorEditor::updateSectionUI()
{
    // Update the UI to reflect the current section
    for (int i = 0; i < 4; ++i)
    {
        if (sections[i].isActive)
        {
            sections[i].titleLabel.setColour(juce::Label::textColourId, juce::Colours::green);
            
            // Display remaining bars in the title
            sections[i].titleLabel.setText("Section " + juce::String(i + 1) + 
                                          " (" + juce::String(sections[i].remainingBars, 1) + " bars)",
                                          juce::dontSendNotification);
        }
        else
        {
            sections[i].titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
            sections[i].titleLabel.setText("Section " + juce::String(i + 1), juce::dontSendNotification);
        }
    }
}

void DrumMachineAudioProcessorEditor::initializeGridForSection(int sectionIndex)
{
    // Get the Game of Life instance
    auto* gameOfLife = gameOfLifeComponent.getGameOfLife();
    if (gameOfLife == nullptr)
        return;
    
    // Get the parameters
    auto* randomizeParam = audioProcessor.getParameterManager().getSectionRandomizeParam(sectionIndex);
    auto* gridStateParam = audioProcessor.getParameterManager().getSectionGridStateParam(sectionIndex);
    auto* densityParam = audioProcessor.getParameterManager().getSectionDensityParam(sectionIndex);
    
    if (randomizeParam == nullptr || gridStateParam == nullptr || densityParam == nullptr)
        return;
    
    // Check if we should randomize or use a specific state
    if (randomizeParam->get())
    {
        // Get the density value (between 0.1 and 0.9)
        float density = densityParam->get();
        
        // Randomize the grid with the specified density
        gameOfLife->initializeWithDensity(density);
        
        // Get the new grid state as a string
        juce::String gridStateStr = gameOfLifeComponent.getGridStateAsString();
        
        // Update the section's grid state text box with the new random state
        sections[sectionIndex].gridStateTextBox.setText(gridStateStr, false);
        
        // Update the parameter
        int gridState = gridStateStr.getIntValue();
        gridStateParam->setValueNotifyingHost(gridState);
    }
    else
    {
        // Use the specified grid state from the parameter
        int gridState = gridStateParam->get();
        juce::String gridStateStr = juce::String(gridState);
        
        // Set the grid state
        gameOfLifeComponent.setGridStateFromString(gridStateStr);
        
        // Update the text box to ensure it matches the parameter
        sections[sectionIndex].gridStateTextBox.setText(gridStateStr, false);
    }
}

void DrumMachineAudioProcessorEditor::textEditorReturnKeyPressed(juce::TextEditor& textEditor)
{
    // Find which section this text editor belongs to
    for (int i = 0; i < 4; ++i)
    {
        if (&textEditor == &sections[i].gridStateTextBox)
        {
            // Update the grid state parameter
            auto* gridStateParam = audioProcessor.getParameterManager().getSectionGridStateParam(i);
            if (gridStateParam != nullptr)
            {
                int gridState = textEditor.getText().getIntValue();
                gridStateParam->setValueNotifyingHost(gridState);
                
                // If this section is active, update the grid immediately
                if (sections[i].isActive)
                {
                    initializeGridForSection(i);
                }
            }
            break;
        }
    }
}

void DrumMachineAudioProcessorEditor::textEditorFocusLost(juce::TextEditor& textEditor)
{
    // Same behavior as return key pressed
    textEditorReturnKeyPressed(textEditor);
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
