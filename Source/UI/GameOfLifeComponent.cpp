#include "GameOfLifeComponent.h"

GameOfLifeComponent::GameOfLifeComponent(ParameterManager& pm)
    : paramManager(pm)
{
    // Initialize MIDI control label
    midiControlLabel.setText("Game of Life is controlled by MIDI notes: ON = Note On, OFF = Note Off", juce::dontSendNotification);
    midiControlLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    midiControlLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(midiControlLabel);
    
    // Initialize randomize button
    randomizeButton.setButtonText("Randomize");
    randomizeButton.addListener(this);
    addAndMakeVisible(randomizeButton);
    
    // Initialize interval controls
    intervalLabel.setText("Update Interval:", juce::dontSendNotification);
    intervalLabel.setFont(juce::Font(14.0f, juce::Font::bold));
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
    
    // Initialize column mapping label
    mappingLabel.setText("Column Sample Mapping:", juce::dontSendNotification);
    mappingLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    addAndMakeVisible(mappingLabel);
    
    // Initialize column control mode label
    modeLabel.setText("Column Control Mode:", juce::dontSendNotification);
    modeLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    addAndMakeVisible(modeLabel);
    
    // Initialize pitch range label
    pitchRangeLabel.setText("Pitch Range: -7 to +8 semitones", juce::dontSendNotification);
    pitchRangeLabel.setFont(juce::Font(12.0f));
    addAndMakeVisible(pitchRangeLabel);
    
    // Initialize column mapping combo boxes
    for (int i = 0; i < ParameterManager::GRID_SIZE; ++i)
    {
        // Create column mapper combo box
        columnMappers[i] = std::make_unique<juce::ComboBox>("Column " + juce::String(i + 1) + " Mapping");
        columnMappers[i]->addItem("None", 1);
        
        for (int j = 0; j < ParameterManager::NUM_DRUM_PADS; ++j)
        {
            columnMappers[i]->addItem("Sample " + juce::String(j + 1), j + 2);
        }
        
        columnMappers[i]->setSelectedItemIndex(0);
        columnMappers[i]->addListener(this);
        addAndMakeVisible(columnMappers[i].get());
        
        // Create attachment
        columnMapperAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            paramManager.getAPVTS(), "colMap" + juce::String(i), *columnMappers[i]);
            
        // Create column control mode combo box
        columnControlModes[i] = std::make_unique<juce::ComboBox>("Column " + juce::String(i + 1) + " Mode");
        columnControlModes[i]->addItem("Velocity", 1);
        columnControlModes[i]->addItem("Pitch", 2);
        columnControlModes[i]->setSelectedItemIndex(0);
        columnControlModes[i]->addListener(this);
        addAndMakeVisible(columnControlModes[i].get());
        
        // Create attachment
        columnControlModeAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            paramManager.getAPVTS(), "colMode" + juce::String(i), *columnControlModes[i]);
    }
    
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
    // Fill background
    g.fillAll(juce::Colours::darkgrey);
    
    // Draw grid
    if (gameOfLife != nullptr)
    {
        // Calculate cell size based on the width of the combo boxes
        int cellSize = getWidth() / ParameterManager::GRID_SIZE;
        
        // Draw each cell
        for (int y = 0; y < ParameterManager::GRID_SIZE; ++y)
        {
            for (int x = 0; x < ParameterManager::GRID_SIZE; ++x)
            {
                bool alive = gameOfLife->getCellState(x, y);
                drawCell(g, x, y, alive, cellSize);
            }
        }
    }
}

void GameOfLifeComponent::resized()
{
    auto area = getLocalBounds();
    
    // Calculate the column width based on the total width
    int columnWidth = area.getWidth() / ParameterManager::GRID_SIZE;
    
    // Position MIDI control label at the top
    auto midiControlLabelArea = area.removeFromTop(30);
    midiControlLabel.setBounds(midiControlLabelArea);
    
    // Position buttons at the top
    auto buttonArea = area.removeFromTop(30);
    randomizeButton.setBounds(buttonArea.removeFromLeft(100));
    
    // Layout for interval label
    auto intervalLabelArea = area.removeFromTop(20);
    intervalLabel.setBounds(intervalLabelArea);
    
    // Layout for interval type combo box
    auto intervalTypeArea = area.removeFromTop(30);
    intervalTypeBox.setBounds(intervalTypeArea.removeFromLeft(100));
    
    // Layout for interval value combo box
    intervalValueBox.setBounds(intervalTypeArea.removeFromLeft(150));
    
    // Layout for column mapping label
    auto labelArea = area.removeFromTop(20);
    mappingLabel.setBounds(labelArea);
    
    // Layout for column mapping combo boxes
    auto mapperArea = area.removeFromTop(30);
    
    for (int i = 0; i < ParameterManager::GRID_SIZE; ++i)
    {
        columnMappers[i]->setBounds(i * columnWidth, mapperArea.getY(), columnWidth - 4, 30);
    }
    
    // Layout for column control mode label
    auto modeLabelArea = area.removeFromTop(20);
    modeLabel.setBounds(modeLabelArea);
    pitchRangeLabel.setBounds(modeLabelArea.translated(300, 0).withWidth(200));
    
    // Layout for column control mode combo boxes
    auto modeArea = area.removeFromTop(30);
    
    for (int i = 0; i < ParameterManager::GRID_SIZE; ++i)
    {
        columnControlModes[i]->setBounds(i * columnWidth, modeArea.getY(), columnWidth - 4, 30);
    }
    
    // Reserve space for the grid (rest of the component)
    // The grid will be drawn in the paint method
}

void GameOfLifeComponent::mouseDown(const juce::MouseEvent& e)
{
    if (gameOfLife == nullptr)
        return;
        
    // Get cell coordinates from mouse position
    int x, y;
    if (getCellCoordinates(e.getPosition(), x, y))
    {
        // Toggle cell state
        gameOfLife->toggleCellState(x, y);
        repaint();
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
        
    // Check if Game of Life is enabled
    if (paramManager.getGameOfLifeEnabledParam()->get())
    {
        // Update Game of Life
        gameOfLife->update();
        
        // Repaint if grid has updated
        if (gameOfLife->hasUpdated())
        {
            gameOfLife->setHasUpdated(false);
            repaint();
        }
    }
}

bool GameOfLifeComponent::getCellCoordinates(const juce::Point<int>& position, int& x, int& y)
{
    // Calculate cell size
    int cellSize = getWidth() / ParameterManager::GRID_SIZE;
    
    // Calculate grid area - starts after the controls
    int controlsHeight = 210; // Approximate height of all controls
    juce::Rectangle<int> gridArea(0, controlsHeight, cellSize * ParameterManager::GRID_SIZE, getHeight() - controlsHeight);
    
    // Check if position is within grid area
    if (gridArea.contains(position))
    {
        // Calculate cell coordinates
        x = position.getX() / cellSize;
        y = (position.getY() - controlsHeight) / cellSize;
        return true;
    }
    
    return false;
}

void GameOfLifeComponent::drawCell(juce::Graphics& g, int x, int y, bool alive, int cellSize)
{
    // Calculate cell position
    int xPos = x * cellSize;
    int yPos = 210 + (y * cellSize); // Start grid after controls
    
    // Draw cell
    juce::Rectangle<int> cellRect(xPos, yPos, cellSize, cellSize);
    cellRect.reduce(1, 1); // Add a small gap between cells
    
    if (alive)
    {
        // Draw active cell
        g.setColour(juce::Colours::lightgreen);
        g.fillRect(cellRect);
        
        // Add highlight
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawLine(xPos + 1, yPos + 1, xPos + cellSize - 2, yPos + 1);
        g.drawLine(xPos + 1, yPos + 1, xPos + 1, yPos + cellSize - 2);
        
        // Add shadow
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.drawLine(xPos + 1, yPos + cellSize - 2, xPos + cellSize - 2, yPos + cellSize - 2);
        g.drawLine(xPos + cellSize - 2, yPos + 1, xPos + cellSize - 2, yPos + cellSize - 2);
    }
    else
    {
        // Draw inactive cell
        g.setColour(juce::Colours::darkgrey.brighter(0.2f));
        g.fillRect(cellRect);
        
        // Add border
        g.setColour(juce::Colours::grey);
        g.drawRect(cellRect);
    }
    
    // Draw column indicator at the top row
    if (y == 0)
    {
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText(juce::String(x + 1), cellRect.withHeight(20).translated(0, -20), juce::Justification::centred, false);
    }
}
