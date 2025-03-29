#pragma once

#include <JuceHeader.h>
#include <array>
#include "ParameterManager.h"
#include "DrumPad.h"
#include "ScaleUtility.h"
#include "Grid.h"

/**
 * Implements Conway's Game of Life cellular automaton with audio triggering
 */
class GameOfLife
{
public:
    // Constructor
    GameOfLife(ParameterManager* pm) : parameterManager(pm)
    {
        // Grid is initialized in its constructor
    }
    
    // Destructor
    ~GameOfLife();
    
    // Initialize the grid
    void initialize(bool randomize = false)
    {
        grid.initialize(randomize);
    }
    
    // Initialize the grid with a specific density (0.0-1.0)
    void initializeWithDensity(float density)
    {
        grid.initializeWithDensity(density);
    }
    
    // Update the grid to the next generation
    void update()
    {
        grid.update();
    }
    
    // Get the state of a cell
    bool getCellState(int x, int y) const
    {
        return grid.getCellState(x, y);
    }
    
    // Set the state of a cell
    void setCellState(int x, int y, bool state)
    {
        grid.setCellState(x, y, state);
    }
    
    // Toggle the state of a cell
    void toggleCellState(int x, int y)
    {
        grid.toggleCellState(x, y);
    }
    
    // Check if a cell has just been activated (was inactive in previous state)
    bool cellJustActivated(int x, int y) const
    {
        return grid.cellJustActivated(x, y);
    }
    
    // Check if a cell has just been deactivated (was active in previous state)
    bool cellJustDeactivated(int x, int y) const
    {
        return grid.cellJustDeactivated(x, y);
    }
    
    // Check if a cell was active in the previous state
    bool wasCellActive(int x, int y) const
    {
        return grid.wasCellActive(x, y);
    }
    
    // Check if the grid has been updated
    bool hasUpdated() const { return grid.hasUpdated(); }
    
    // Check for active cells and trigger samples
    template<typename SampleMapFunc, typename ModeMapFunc>
    void checkAndTriggerSamples(std::array<DrumPad, ParameterManager::NUM_SAMPLES>& drumPads, 
                                int numPads,
                                SampleMapFunc getSampleForColumn,
                                ModeMapFunc getControlModeForColumn,
                                int midiNoteOffset = 60);
    
    // Update only the pitch of pitched columns without retriggering samples
    template<typename SampleMapFunc, typename ModeMapFunc>
    void updatePitchOnly(std::array<DrumPad, ParameterManager::NUM_SAMPLES>& drumPads, 
                         int numPads,
                         SampleMapFunc getSampleForColumn,
                         ModeMapFunc getControlModeForColumn,
                         int midiNoteOffset = 60);
    
    // Check for inactive cells and stop samples
    template<typename SampleMapFunc>
    void checkAndStopSamples(std::array<DrumPad, ParameterManager::NUM_SAMPLES>& drumPads, 
                             int numPads,
                             SampleMapFunc getSampleForColumn);
    
private:
    ParameterManager* parameterManager;
    GameOfLifeApp::Grid grid;
};

// Template method implementations
template<typename SampleMapFunc, typename ModeMapFunc>
void GameOfLife::checkAndTriggerSamples(std::array<DrumPad, ParameterManager::NUM_SAMPLES>& drumPads, 
                                        int numPads,
                                        SampleMapFunc getSampleForColumn,
                                        ModeMapFunc getControlModeForColumn,
                                        int midiNoteOffset)
{
    // Only process if the grid has been updated
    if (!grid.hasUpdated())
        return;
    
    // Check each cell in the grid
    for (int y = 0; y < ParameterManager::GRID_SIZE; ++y)
    {
        for (int x = 0; x < ParameterManager::GRID_SIZE; ++x)
        {
            // Check if this cell just became active
            if (grid.cellJustActivated(x, y))
            {
                // Get the sample index for this column
                int sampleIndex = getSampleForColumn(x);
                
                // Skip if the sample index is invalid
                if (sampleIndex < 0 || sampleIndex >= numPads)
                    continue;
                
                // Get the control mode for this column
                auto controlMode = getControlModeForColumn(x);
                
                // Calculate velocity based on y position (higher = louder)
                float velocity = (float)(y + 1) / (float)ParameterManager::GRID_SIZE;
                
                // Trigger the sample based on control mode
                if (controlMode == ColumnControlMode::Pitch)
                {
                    // Calculate MIDI note based on y position (higher = higher pitch)
                    int midiNote = midiNoteOffset + y;
                    
                    // Convert MIDI note to pitch shift in semitones
                    int pitchShift = midiNote - 60; // Assuming 60 (C4) is the base note
                    
                    // Apply scale correction if needed
                    MusicalScale scale = parameterManager->getSelectedScale();
                    pitchShift = ScaleUtility::snapToScale(pitchShift, 0, scale);
                    
                    // Trigger the sample with pitch shift
                    drumPads[sampleIndex].triggerSampleWithPitchForCell(velocity, pitchShift, x, y);
                }
                else
                {
                    // Default to volume mode
                    drumPads[sampleIndex].triggerSampleForCell(velocity, x, y);
                }
            }
        }
    }
}

template<typename SampleMapFunc, typename ModeMapFunc>
void GameOfLife::updatePitchOnly(std::array<DrumPad, ParameterManager::NUM_SAMPLES>& drumPads, 
                                 int numPads,
                                 SampleMapFunc getSampleForColumn,
                                 ModeMapFunc getControlModeForColumn,
                                 int midiNoteOffset)
{
    // Check each cell in the grid
    for (int y = 0; y < ParameterManager::GRID_SIZE; ++y)
    {
        for (int x = 0; x < ParameterManager::GRID_SIZE; ++x)
        {
            // Only process active cells
            if (grid.getCellState(x, y))
            {
                // Get the sample index for this column
                int sampleIndex = getSampleForColumn(x);
                
                // Skip if the sample index is invalid
                if (sampleIndex < 0 || sampleIndex >= numPads)
                    continue;
                
                // Get the control mode for this column
                auto controlMode = getControlModeForColumn(x);
                
                // Only update pitch for pitch mode columns
                if (controlMode == ColumnControlMode::Pitch)
                {
                    // Calculate MIDI note based on y position (higher = higher pitch)
                    int midiNote = midiNoteOffset + y;
                    
                    // Convert MIDI note to pitch shift in semitones
                    int pitchShift = midiNote - 60; // Assuming 60 (C4) is the base note
                    
                    // Apply scale correction if needed
                    MusicalScale scale = parameterManager->getSelectedScale();
                    pitchShift = ScaleUtility::snapToScale(pitchShift, 0, scale);
                    
                    // Update the pitch for this cell
                    drumPads[sampleIndex].updatePitchForCell(pitchShift, x, y);
                }
            }
        }
    }
}

template<typename SampleMapFunc>
void GameOfLife::checkAndStopSamples(std::array<DrumPad, ParameterManager::NUM_SAMPLES>& drumPads, 
                                     int numPads,
                                     SampleMapFunc getSampleForColumn)
{
    // Only process if the grid has been updated
    if (!grid.hasUpdated())
        return;
    
    // Check each cell in the grid
    for (int y = 0; y < ParameterManager::GRID_SIZE; ++y)
    {
        for (int x = 0; x < ParameterManager::GRID_SIZE; ++x)
        {
            // Check if this cell just became inactive
            if (grid.cellJustDeactivated(x, y))
            {
                // Get the sample index for this column
                int sampleIndex = getSampleForColumn(x);
                
                // Skip if the sample index is invalid
                if (sampleIndex < 0 || sampleIndex >= numPads)
                    continue;
                
                // Stop the sample for this cell
                drumPads[sampleIndex].stopSampleForCell(x, y);
            }
        }
    }
}
