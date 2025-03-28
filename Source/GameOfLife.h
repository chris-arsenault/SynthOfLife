#pragma once

#include <JuceHeader.h>
#include "DrumPad.h"
#include "ParameterManager.h"

class GameOfLife
{
public:
    // Using ParameterManager::GRID_SIZE instead of defining our own
    
    GameOfLife();
    ~GameOfLife();
    
    // Initialize the grid
    void initialize(bool randomize = true);
    
    // Update the grid based on Game of Life rules
    void update();
    
    // Get/Set cell state
    bool getCellState(int x, int y) const;
    void setCellState(int x, int y, bool state);
    void toggleCellState(int x, int y);
    
    // Check for newly activated cells and trigger samples
    template<typename SampleMapFunc, typename ModeMapFunc>
    void checkAndTriggerSamples(std::array<DrumPad, ParameterManager::NUM_DRUM_PADS>& drumPads, 
                               int numPads,
                               SampleMapFunc getSampleForColumn,
                               ModeMapFunc getControlModeForColumn);
    
    // Has the grid been updated since last check?
    bool hasUpdated() const { return gridHasUpdated; }
    void setHasUpdated(bool value) { gridHasUpdated = value; }
    
private:
    // Grid of cells
    bool grid[ParameterManager::GRID_SIZE][ParameterManager::GRID_SIZE];
    bool previousGrid[ParameterManager::GRID_SIZE][ParameterManager::GRID_SIZE];
    bool nextGrid[ParameterManager::GRID_SIZE][ParameterManager::GRID_SIZE];
    
    // Has the grid been updated since last check?
    bool gridHasUpdated;
    
    // Count live neighbors
    int countLiveNeighbors(int x, int y) const;
};

// Template implementation
template<typename SampleMapFunc, typename ModeMapFunc>
void GameOfLife::checkAndTriggerSamples(std::array<DrumPad, ParameterManager::NUM_DRUM_PADS>& drumPads, 
                                       int numPads,
                                       SampleMapFunc getSampleForColumn,
                                       ModeMapFunc getControlModeForColumn)
{
    // Debug output
    DBG("Checking for newly activated cells in Game of Life grid");
    
    // Track if any samples were triggered
    bool anySampleTriggered = false;
    
    // Check for newly activated cells in each column
    for (int x = 0; x < ParameterManager::GRID_SIZE; ++x)
    {
        // Get the sample index for this column
        int sampleIndex = getSampleForColumn(x);
        
        // Skip if no sample is mapped to this column
        if (sampleIndex < 0 || sampleIndex >= numPads)
            continue;
            
        for (int y = 0; y < ParameterManager::GRID_SIZE; ++y)
        {
            // If cell was just activated (current = true, previous = false)
            if (grid[y][x] && !previousGrid[y][x])
            {
                // Debug output
                DBG("Cell activated at (" + juce::String(x) + ", " + juce::String(y) + 
                    ") mapped to sample index: " + juce::String(sampleIndex));
                
                // Get control mode for this column
                ColumnControlMode mode = getControlModeForColumn(x);
                
                // Calculate velocity based on y position (higher = louder)
                float velocity = static_cast<float>(y + 1) / static_cast<float>(ParameterManager::GRID_SIZE);
                
                // Trigger sample based on control mode
                if (mode == ColumnControlMode::Velocity)
                {
                    // Trigger with velocity
                    drumPads[sampleIndex].triggerSample(velocity);
                    anySampleTriggered = true;
                }
                else if (mode == ColumnControlMode::Pitch)
                {
                    // Calculate pitch shift based on y position
                    // Map y (0 to GRID_SIZE-1) to pitch shift (-7 to +8 semitones)
                    int pitchShift = static_cast<int>((static_cast<float>(y) / (ParameterManager::GRID_SIZE - 1)) * 16.0f) - 7;
                    
                    // Trigger with pitch shift
                    drumPads[sampleIndex].triggerSampleWithPitch(velocity, pitchShift);
                    anySampleTriggered = true;
                }
            }
        }
    }
    
    // Debug output if no samples were triggered
    if (!anySampleTriggered) {
        DBG("No samples were triggered during this update");
    }
}
