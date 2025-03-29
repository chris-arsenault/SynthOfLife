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
    
    // Initialize clear button
    clearButton.setButtonText("Clear");
    clearButton.addListener(this);
    addAndMakeVisible(clearButton);
    
    // Initialize grid state text box and label
    gridStateLabel.setText("Grid State:", juce::dontSendNotification);
    gridStateLabel.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 14.0f, juce::Font::bold));
    addAndMakeVisible(gridStateLabel);
    
    gridStateTextBox.setMultiLine(false);
    gridStateTextBox.setReturnKeyStartsNewLine(false);
    gridStateTextBox.setReadOnly(false);
    gridStateTextBox.setScrollbarsShown(true);
    gridStateTextBox.setCaretVisible(true);
    gridStateTextBox.setPopupMenuEnabled(true);
    gridStateTextBox.setText("0");
    gridStateTextBox.onReturnKey = [this]() {
        setGridStateFromString(gridStateTextBox.getText());
        return true;
    };
    gridStateTextBox.onFocusLost = [this]() {
        setGridStateFromString(gridStateTextBox.getText());
    };
    addAndMakeVisible(gridStateTextBox);
    
    // Interval controls have been moved to the main tab
    
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
    
    // Position the buttons and text box
    auto controlsArea = area.removeFromTop(30);
    
    // Clear button on the left
    clearButton.setBounds(controlsArea.removeFromLeft(120));
    
    // Randomize button in the middle
    randomizeButton.setBounds(controlsArea.removeFromLeft(120));
    
    // Grid state label and text box on the right
    auto gridStateLabelArea = controlsArea.removeFromLeft(80);
    gridStateLabel.setBounds(gridStateLabelArea);
    gridStateTextBox.setBounds(controlsArea.removeFromLeft(200));
    
    // Interval controls have been moved to the main tab
    
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
        
        // Update the grid state text box
        gridStateTextBox.setText(getGridStateAsString(), false);
        
        repaint();
    }
    else if (button == &clearButton)
    {
        // Clear the grid (initialize without randomizing)
        if (gameOfLife != nullptr)
        {
            for (int x = 0; x < ParameterManager::GRID_SIZE; ++x)
            {
                for (int y = 0; y < ParameterManager::GRID_SIZE; ++y)
                {
                    gameOfLife->setCellState(x, y, false);
                }
            }
            
            // Update the grid state text box
            gridStateTextBox.setText("0", false);
            
            repaint();
        }
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
    
    // Update the grid state text box if the grid has changed
    if (gameOfLife->hasUpdated())
    {
        gridStateTextBox.setText(getGridStateAsString(), false);
    }
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

juce::String GameOfLifeComponent::getGridStateAsString() const
{
    if (gameOfLife == nullptr)
        return "0";
    
    // Convert the grid state to a binary string
    juce::String binaryString;
    
    for (int y = 0; y < ParameterManager::GRID_SIZE; ++y)
    {
        for (int x = 0; x < ParameterManager::GRID_SIZE; ++x)
        {
            binaryString += gameOfLife->getCellState(x, y) ? "1" : "0";
        }
    }
    
    // Convert binary string to a decimal integer string
    juce::BigInteger bigInt;
    bigInt.parseString(binaryString, 2);
    
    return bigInt.toString(10);
}

void GameOfLifeComponent::setGridStateFromString(const juce::String& stateString)
{
    if (gameOfLife == nullptr)
        return;
    
    // Parse the string as a decimal integer
    juce::BigInteger bigInt;
    bigInt.parseString(stateString, 10);
    
    // Check if the parsed value is valid (non-zero or explicitly "0")
    bool validNumber = (bigInt != 0) || stateString == "0";
    
    if (!validNumber)
    {
        // If invalid, reset to 0
        gridStateTextBox.setText("0", false);
        
        // Clear the grid
        for (int x = 0; x < ParameterManager::GRID_SIZE; ++x)
        {
            for (int y = 0; y < ParameterManager::GRID_SIZE; ++y)
            {
                gameOfLife->setCellState(x, y, false);
            }
        }
        
        repaint();
        return;
    }
    
    // Convert to binary string, padded to grid size
    juce::String binaryString = bigInt.toString(2);
    
    // Pad with zeros to match grid size
    int requiredLength = ParameterManager::GRID_SIZE * ParameterManager::GRID_SIZE;
    while (binaryString.length() < requiredLength)
    {
        binaryString = "0" + binaryString;
    }
    
    // If too long, truncate to grid size
    if (binaryString.length() > requiredLength)
    {
        binaryString = binaryString.substring(binaryString.length() - requiredLength);
    }
    
    // Set the grid state based on the binary string
    int index = 0;
    for (int y = 0; y < ParameterManager::GRID_SIZE; ++y)
    {
        for (int x = 0; x < ParameterManager::GRID_SIZE; ++x)
        {
            bool cellState = binaryString[index] == '1';
            gameOfLife->setCellState(x, y, cellState);
            index++;
        }
    }
    
    // Update the text box with the normalized value
    gridStateTextBox.setText(getGridStateAsString(), false);
    
    repaint();
}
