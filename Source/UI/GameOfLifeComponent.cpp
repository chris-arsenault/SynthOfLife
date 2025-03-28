#include "GameOfLifeComponent.h"

GameOfLifeComponent::GameOfLifeComponent(ParameterManager& pm)
    : paramManager(pm)
{
    // Initialize MIDI control label
    midiControlLabel.setText("Game of Life is controlled by MIDI notes: ON = Note On, OFF = Note Off", juce::dontSendNotification);
    midiControlLabel.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 14.0f, juce::Font::bold));
    midiControlLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(midiControlLabel);
    
    // Initialize randomize button
    randomizeButton.setButtonText("Randomize");
    randomizeButton.addListener(this);
    addAndMakeVisible(randomizeButton);
    
    // Initialize interval controls
    intervalLabel.setText("Update Interval:", juce::dontSendNotification);
    intervalLabel.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 14.0f, juce::Font::bold));
    addAndMakeVisible(intervalLabel);
    
    // Set up interval type combo box
    intervalTypeBox.addItem("Normal", 1);
    intervalTypeBox.addItem("Dotted", 2);
    intervalTypeBox.addItem("Triplet", 3);
    intervalTypeBox.setSelectedItemIndex(0);
    intervalTypeBox.addListener(this);
    addAndMakeVisible(intervalTypeBox);
    
    // Set up interval value combo box
    intervalValueBox.addItem("1/4 (Quarter)", 1);
    intervalValueBox.addItem("1/8 (Eighth)", 2);
    intervalValueBox.addItem("1/16 (Sixteenth)", 3);
    intervalValueBox.setSelectedItemIndex(2);
    intervalValueBox.addListener(this);
    addAndMakeVisible(intervalValueBox);
    
    // Create attachments for interval controls
    intervalTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        paramManager.getAPVTS(), "intervalType", intervalTypeBox);
    intervalValueAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        paramManager.getAPVTS(), "intervalValue", intervalValueBox);
    
    // Create parameter attachments
    randomizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        paramManager.getAPVTS(), "golRandomize", randomizeButton);
    
    // Start the timer for Game of Life updates
    startTimerHz(4); // Update 4 times per second
}

GameOfLifeComponent::~GameOfLifeComponent()
{
    stopTimer();
}

void GameOfLifeComponent::paint(juce::Graphics& g)
{
    // Fill the background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    // Draw the Game of Life grid
    if (gameOfLife != nullptr)
    {
        // Calculate the grid size
        auto area = getLocalBounds().reduced(10);
        int gridSize = juce::jmin(area.getWidth(), area.getHeight() - 100); // Reserve space for controls
        int cellSize = gridSize / ParameterManager::GRID_SIZE;
        
        // Center the grid horizontally
        int gridX = (getWidth() - (cellSize * ParameterManager::GRID_SIZE)) / 2;
        
        // Position the grid below the controls
        int gridY = 100; // Approximate space used by controls
        
        // Draw the grid
        for (int x = 0; x < ParameterManager::GRID_SIZE; ++x)
        {
            for (int y = 0; y < ParameterManager::GRID_SIZE; ++y)
            {
                bool cellState = gameOfLife->getCellState(x, y);
                drawCell(g, x, y, cellState, cellSize);
            }
        }
        
        // Draw grid lines
        g.setColour(juce::Colours::grey);
        
        // Draw horizontal grid lines
        for (int y = 0; y <= ParameterManager::GRID_SIZE; ++y)
        {
            g.drawLine(gridX, gridY + y * cellSize, 
                      gridX + ParameterManager::GRID_SIZE * cellSize, gridY + y * cellSize);
        }
        
        // Draw vertical grid lines
        for (int x = 0; x <= ParameterManager::GRID_SIZE; ++x)
        {
            g.drawLine(gridX + x * cellSize, gridY, 
                      gridX + x * cellSize, gridY + ParameterManager::GRID_SIZE * cellSize);
        }
    }
}

void GameOfLifeComponent::resized()
{
    // Calculate the available area
    auto area = getLocalBounds().reduced(10);
    
    // Calculate the grid size
    int gridSize = juce::jmin(area.getWidth(), area.getHeight() - 100); // Reserve space for controls
    int cellSize = gridSize / ParameterManager::GRID_SIZE;
    
    // Center the grid horizontally
    int gridX = (getWidth() - (cellSize * ParameterManager::GRID_SIZE)) / 2;
    
    // Position the MIDI control label at the top
    auto labelArea = area.removeFromTop(30);
    midiControlLabel.setBounds(labelArea);
    
    // Position the randomize button
    auto buttonArea = area.removeFromTop(30);
    randomizeButton.setBounds(buttonArea.withWidth(120));
    
    // Position the interval controls
    auto intervalArea = area.removeFromTop(30);
    
    // Layout for interval label
    auto intervalLabelArea = intervalArea.removeFromLeft(120);
    intervalLabel.setBounds(intervalLabelArea);
    
    // Layout for interval type combo box
    auto intervalTypeArea = intervalArea;
    intervalTypeBox.setBounds(intervalTypeArea.removeFromLeft(150));
    
    // Layout for interval value combo box
    intervalValueBox.setBounds(intervalTypeArea.removeFromLeft(150));
    
    // Reserve space for the grid (rest of the component)
    // The grid will be drawn in the paint method
}

void GameOfLifeComponent::mouseDown(const juce::MouseEvent& e)
{
    // Check if the Game of Life model is available
    if (gameOfLife != nullptr)
    {
        // Get the cell coordinates from the mouse position
        int x, y;
        if (getCellCoordinates(e.getPosition(), x, y))
        {
            // Toggle the cell state
            bool currentState = gameOfLife->getCellState(x, y);
            gameOfLife->setCellState(x, y, !currentState);
            
            // Trigger a repaint
            repaint();
        }
    }
}

void GameOfLifeComponent::buttonClicked(juce::Button* button)
{
    if (gameOfLife == nullptr)
        return;
        
    if (button == &randomizeButton)
    {
        // Randomize the grid
        gameOfLife->initialize(true);
        repaint();
    }
}

void GameOfLifeComponent::comboBoxChanged(juce::ComboBox* comboBox)
{
    // Handle combo box changes if needed
    repaint();
}

void GameOfLifeComponent::timerCallback()
{
    if (gameOfLife == nullptr)
        return;
        
    // Get the processor
    auto* processor = dynamic_cast<DrumMachineAudioProcessor*>(paramManager.getProcessor());
    if (processor == nullptr)
        return;
        
    // Always repaint to update the UI
    repaint();
}

bool GameOfLifeComponent::getCellCoordinates(const juce::Point<int>& position, int& x, int& y)
{
    // Calculate the grid size
    auto area = getLocalBounds().reduced(10);
    int gridSize = juce::jmin(area.getWidth(), area.getHeight() - 100); // Reserve space for controls
    int cellSize = gridSize / ParameterManager::GRID_SIZE;
    
    // Center the grid horizontally
    int gridX = (getWidth() - (cellSize * ParameterManager::GRID_SIZE)) / 2;
    
    // Position the grid below the controls
    int gridY = 100; // Approximate space used by controls
    
    // Calculate the grid bounds
    juce::Rectangle<int> gridBounds(gridX, gridY, 
                                    cellSize * ParameterManager::GRID_SIZE, 
                                    cellSize * ParameterManager::GRID_SIZE);
    
    // Check if the position is within the grid
    if (gridBounds.contains(position))
    {
        // Calculate the cell coordinates
        x = (position.x - gridX) / cellSize;
        y = (position.y - gridY) / cellSize;
        
        // Ensure the coordinates are within the grid
        return (x >= 0 && x < ParameterManager::GRID_SIZE && y >= 0 && y < ParameterManager::GRID_SIZE);
    }
    
    return false;
}

void GameOfLifeComponent::drawCell(juce::Graphics& g, int x, int y, bool alive, int cellSize)
{
    // Calculate the grid size
    auto area = getLocalBounds().reduced(10);
    int gridSize = juce::jmin(area.getWidth(), area.getHeight() - 100); // Reserve space for controls
    
    // Center the grid horizontally
    int gridX = (getWidth() - (cellSize * ParameterManager::GRID_SIZE)) / 2;
    
    // Position the grid below the controls
    int gridY = 100; // Approximate space used by controls
    
    // Calculate cell position
    int cellX = gridX + x * cellSize;
    int cellY = gridY + y * cellSize;
    
    // Draw cell background
    if (alive)
    {
        // Get the sample index for this column
        int sampleIndex = paramManager.getSampleForColumn(x);
        
        // Choose color based on the sample index
        juce::Colour cellColor;
        
        if (sampleIndex >= 0)
        {
            // Use a color based on the sample index
            float hue = (float)sampleIndex / ParameterManager::NUM_SAMPLES;
            cellColor = juce::Colour::fromHSV(hue, 0.8f, 0.9f, 1.0f);
        }
        else
        {
            // Default color for unmapped columns
            cellColor = juce::Colours::lightgreen;
        }
        
        g.setColour(cellColor);
    }
    else
    {
        g.setColour(juce::Colours::black);
    }
    
    g.fillRect(cellX + 1, cellY + 1, cellSize - 2, cellSize - 2);
    
    // Draw cell border
    g.setColour(juce::Colours::grey);
    g.drawRect(cellX, cellY, cellSize, cellSize, 1);
}
