#pragma once

#include <JuceHeader.h>
#include "ParameterManager.h"

namespace GameOfLifeApp {

/**
 * Represents a cellular automaton grid with Game of Life rules
 */
class Grid
{
public:
    Grid();
    ~Grid() = default;
    
    // Initialize the grid
    void initialize(bool randomize = false);
    
    // Update the grid to the next generation
    void update();
    
    // Get the state of a cell
    bool getCellState(int x, int y) const;
    
    // Set the state of a cell
    void setCellState(int x, int y, bool state);
    
    // Toggle the state of a cell
    void toggleCellState(int x, int y);
    
    // Check if a cell has just become active (was inactive in previous grid)
    bool cellJustActivated(int x, int y) const;
    
    // Check if a cell has just become inactive (was active in previous grid)
    bool cellJustDeactivated(int x, int y) const;
    
    // Check if a cell was active in the previous grid
    bool wasCellActive(int x, int y) const;
    
    // Check if the grid has been updated since last check
    bool hasUpdated() const;
    
private:
    // Count the number of live neighbors for a cell
    int countLiveNeighbors(int x, int y) const;
    
    // Grid data - current, next, and previous states
    bool grid[ParameterManager::GRID_SIZE][ParameterManager::GRID_SIZE];
    bool nextGrid[ParameterManager::GRID_SIZE][ParameterManager::GRID_SIZE];
    bool previousGrid[ParameterManager::GRID_SIZE][ParameterManager::GRID_SIZE];
    
    // Flag to track if the grid has been updated
    bool gridHasUpdated;
};

} // namespace GameOfLifeApp
