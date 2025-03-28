#pragma once

#include <JuceHeader.h>
#include "DrumPad.h"
#include "ParameterManager.h"
#include "ScaleUtility.h"

class GameOfLife
{
public:
    // Using ParameterManager::GRID_SIZE instead of defining our own
    
    GameOfLife(ParameterManager* paramManager = nullptr) : parameterManager(paramManager)
    {
        // Initialize grid to all false
        for (auto& row : grid)
            std::fill(row.begin(), row.end(), false);
            
        // Initialize previous grid
        for (auto& row : previousGrid)
            std::fill(row.begin(), row.end(), false);
            
        // Initialize grid has updated flag
        gridHasUpdated = false;
    }
    
    ~GameOfLife();
    
    // Initialize the grid
    void initialize(bool randomize = true);
    
    // Update the grid based on Game of Life rules
    void update();
    
    // Get/Set cell state
    bool getCellState(int x, int y) const;
    void setCellState(int x, int y, bool state);
    void toggleCellState(int x, int y);
    
    // Check for active cells and trigger samples
    template<typename SampleMapFunc, typename ModeMapFunc>
    void checkAndTriggerSamples(std::array<DrumPad, ParameterManager::NUM_DRUM_PADS>& drumPads, 
                               int numPads,
                               SampleMapFunc getSampleForColumn,
                               ModeMapFunc getControlModeForColumn,
                               int midiNoteOffset = 60);
    
    // Check for deactivated cells and stop samples
    template<typename SampleMapFunc>
    void checkAndStopSamples(std::array<DrumPad, ParameterManager::NUM_DRUM_PADS>& drumPads, 
                            int numPads,
                            SampleMapFunc getSampleForColumn);
    
    // Has the grid been updated since last check?
    bool hasUpdated() const { return gridHasUpdated; }
    void setHasUpdated(bool value) { gridHasUpdated = value; }
    
private:
    // Grid of cells
    std::array<std::array<bool, ParameterManager::GRID_SIZE>, ParameterManager::GRID_SIZE> grid;
    std::array<std::array<bool, ParameterManager::GRID_SIZE>, ParameterManager::GRID_SIZE> previousGrid;
    std::array<std::array<bool, ParameterManager::GRID_SIZE>, ParameterManager::GRID_SIZE> nextGrid;
    
    // Has the grid been updated since last check?
    bool gridHasUpdated;
    
    // Reference to the parameter manager
    ParameterManager* parameterManager;
    
    // Count live neighbors
    int countLiveNeighbors(int x, int y) const;
};

// Template implementation
template<typename SampleMapFunc, typename ModeMapFunc>
void GameOfLife::checkAndTriggerSamples(std::array<DrumPad, ParameterManager::NUM_DRUM_PADS>& drumPads, 
                                       int numPads,
                                       SampleMapFunc getSampleForColumn,
                                       ModeMapFunc getControlModeForColumn,
                                       int midiNoteOffset)
{
    // Debug output
    DBG("Checking for active cells in Game of Life grid");
    
    // Track if any samples were triggered
    bool anySampleTriggered = false;
    
    // Calculate MIDI note offset from C3 (60)
    int noteOffsetFromC3 = midiNoteOffset - 60;
    
    // Check for active cells in each column
    for (int x = 0; x < ParameterManager::GRID_SIZE; ++x)
    {
        // Get the sample index for this column
        int sampleIndex = getSampleForColumn(x);
        
        // Skip if no sample is mapped to this column
        if (sampleIndex < 0 || sampleIndex >= numPads)
            continue;
            
        for (int y = 0; y < ParameterManager::GRID_SIZE; ++y)
        {
            // Trigger samples for all active cells, not just newly activated ones
            if (grid[y][x])
            {
                // Debug output
                DBG("Active cell at (" + juce::String(x) + ", " + juce::String(y) + 
                    ") mapped to sample index: " + juce::String(sampleIndex));
                
                // Get control mode for this column
                ColumnControlMode mode = getControlModeForColumn(x);
                
                // Calculate velocity based on y position (higher = louder)
                float velocity = static_cast<float>(y + 1) / static_cast<float>(ParameterManager::GRID_SIZE);
                
                // Trigger sample based on control mode
                if (mode == ColumnControlMode::Velocity)
                {
                    // Trigger with velocity
                    drumPads[sampleIndex].triggerSampleForCell(velocity, x, y);
                    anySampleTriggered = true;
                }
                else if (mode == ColumnControlMode::Pitch)
                {
                    // Get the current musical scale
                    MusicalScale currentScale = parameterManager->getSelectedScale();
                    
                    // Invert y position so higher rows produce higher pitches
                    // (0 is the top of the grid in the UI, but we want higher y values to be higher pitches)
                    int invertedY = ParameterManager::GRID_SIZE - y - 1;
                    
                    // Calculate pitch shift based on y position and the selected scale
                    int scalePitchShift = ScaleUtility::getPitchShiftForPosition(
                        currentScale, 
                        invertedY, 
                        ParameterManager::GRID_SIZE
                    );
                    
                    // Add the MIDI note offset to the scale-based pitch shift
                    int totalPitchShift = scalePitchShift + noteOffsetFromC3;
                    
                    // Debug output
                    DBG("Using scale: " + juce::String((int)currentScale) + 
                        ", position: " + juce::String(y) + 
                        ", inverted position: " + juce::String(invertedY) +
                        ", scale pitch shift: " + juce::String(scalePitchShift) +
                        ", MIDI note offset: " + juce::String(noteOffsetFromC3) +
                        ", total pitch shift: " + juce::String(totalPitchShift));
                    
                    // Trigger with total pitch shift
                    drumPads[sampleIndex].triggerSampleWithPitchForCell(velocity, totalPitchShift, x, y);
                    anySampleTriggered = true;
                }
            }
        }
    }
    
    // Debug output
    if (anySampleTriggered)
    {
        DBG("Triggered samples for active cells in Game of Life grid");
    }
    else
    {
        DBG("No samples triggered for Game of Life grid update");
    }
}

// Template implementation for stopping samples
template<typename SampleMapFunc>
void GameOfLife::checkAndStopSamples(std::array<DrumPad, ParameterManager::NUM_DRUM_PADS>& drumPads, 
                                    int numPads,
                                    SampleMapFunc getSampleForColumn)
{
    // Debug output
    DBG("Checking for deactivated cells in Game of Life grid");
    
    // Track if any samples were stopped
    bool anySampleStopped = false;
    
    // Check for deactivated cells in each column
    for (int x = 0; x < ParameterManager::GRID_SIZE; ++x)
    {
        // Get the sample index for this column
        int sampleIndex = getSampleForColumn(x);
        
        // Skip if no sample is mapped to this column
        if (sampleIndex < 0 || sampleIndex >= numPads)
            continue;
        
        for (int y = 0; y < ParameterManager::GRID_SIZE; ++y)
        {
            // If cell was just deactivated (current = false, previous = true)
            if (!grid[y][x] && previousGrid[y][x])
            {
                // Debug output
                DBG("Cell deactivated at (" + juce::String(x) + ", " + juce::String(y) + 
                    ") mapped to sample index: " + juce::String(sampleIndex));
                
                // Stop the sample for this specific cell
                drumPads[sampleIndex].stopSampleForCell(x, y);
                anySampleStopped = true;
            }
        }
    }
    
    // Debug output
    if (anySampleStopped)
    {
        DBG("Stopped samples for deactivated cells in Game of Life grid");
    }
    else
    {
        DBG("No samples stopped for Game of Life grid update");
    }
}
