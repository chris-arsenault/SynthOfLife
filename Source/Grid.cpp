#include "Grid.h"

namespace GameOfLifeApp {

Grid::Grid() : gridHasUpdated(false)
{
    // Initialize grid to all cells dead
    initialize(false);
}

void Grid::initialize(bool randomize)
{
    // Clear the grid first
    for (int y = 0; y < ParameterManager::GRID_SIZE; ++y)
    {
        for (int x = 0; x < ParameterManager::GRID_SIZE; ++x)
        {
            grid[y][x] = false;
            nextGrid[y][x] = false;
            previousGrid[y][x] = false;
        }
    }
    
    // If randomize is true, set random cells to alive
    if (randomize)
    {
        auto& random = juce::Random::getSystemRandom();
        
        for (int y = 0; y < ParameterManager::GRID_SIZE; ++y)
        {
            for (int x = 0; x < ParameterManager::GRID_SIZE; ++x)
            {
                // About 25% chance of a cell being alive initially
                grid[y][x] = (random.nextInt(100) < 25);
                previousGrid[y][x] = grid[y][x];
            }
        }
    }
    
    gridHasUpdated = true;
}

void Grid::update()
{
    // Save current grid state to previous grid
    for (int y = 0; y < ParameterManager::GRID_SIZE; ++y)
    {
        for (int x = 0; x < ParameterManager::GRID_SIZE; ++x)
        {
            previousGrid[y][x] = grid[y][x];
        }
    }
    
    // Apply Game of Life rules to calculate next grid state
    for (int y = 0; y < ParameterManager::GRID_SIZE; ++y)
    {
        for (int x = 0; x < ParameterManager::GRID_SIZE; ++x)
        {
            int liveNeighbors = countLiveNeighbors(x, y);
            
            // Apply Conway's Game of Life rules
            if (grid[y][x])
            {
                // Live cell with fewer than 2 live neighbors dies (underpopulation)
                // Live cell with more than 3 live neighbors dies (overpopulation)
                nextGrid[y][x] = (liveNeighbors == 2 || liveNeighbors == 3);
            }
            else
            {
                // Dead cell with exactly 3 live neighbors becomes alive (reproduction)
                nextGrid[y][x] = (liveNeighbors == 3);
            }
        }
    }
    
    // Update the current grid with the calculated next state
    for (int y = 0; y < ParameterManager::GRID_SIZE; ++y)
    {
        for (int x = 0; x < ParameterManager::GRID_SIZE; ++x)
        {
            grid[y][x] = nextGrid[y][x];
        }
    }
    
    gridHasUpdated = true;
}

int Grid::countLiveNeighbors(int x, int y) const
{
    int count = 0;
    
    // Check all 8 neighboring cells
    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            // Skip the cell itself
            if (dx == 0 && dy == 0)
                continue;
            
            // Calculate neighbor coordinates with wrapping
            int nx = (x + dx + ParameterManager::GRID_SIZE) % ParameterManager::GRID_SIZE;
            int ny = (y + dy + ParameterManager::GRID_SIZE) % ParameterManager::GRID_SIZE;
            
            // Count live neighbors
            if (grid[ny][nx])
                count++;
        }
    }
    
    return count;
}

bool Grid::getCellState(int x, int y) const
{
    // Ensure coordinates are within bounds
    x = (x + ParameterManager::GRID_SIZE) % ParameterManager::GRID_SIZE;
    y = (y + ParameterManager::GRID_SIZE) % ParameterManager::GRID_SIZE;
    
    return grid[y][x];
}

void Grid::setCellState(int x, int y, bool state)
{
    // Ensure coordinates are within bounds
    x = (x + ParameterManager::GRID_SIZE) % ParameterManager::GRID_SIZE;
    y = (y + ParameterManager::GRID_SIZE) % ParameterManager::GRID_SIZE;
    
    grid[y][x] = state;
    previousGrid[y][x] = state; // Update previous grid to avoid false triggers
    gridHasUpdated = true;
}

void Grid::toggleCellState(int x, int y)
{
    // Ensure coordinates are within bounds
    x = (x + ParameterManager::GRID_SIZE) % ParameterManager::GRID_SIZE;
    y = (y + ParameterManager::GRID_SIZE) % ParameterManager::GRID_SIZE;
    
    grid[y][x] = !grid[y][x];
    previousGrid[y][x] = grid[y][x]; // Update previous grid to avoid false triggers
    gridHasUpdated = true;
}

bool Grid::cellJustActivated(int x, int y) const
{
    // Ensure coordinates are within bounds
    x = (x + ParameterManager::GRID_SIZE) % ParameterManager::GRID_SIZE;
    y = (y + ParameterManager::GRID_SIZE) % ParameterManager::GRID_SIZE;
    
    // Cell is active now but was not active in the previous grid
    return grid[y][x] && !previousGrid[y][x];
}

bool Grid::cellJustDeactivated(int x, int y) const
{
    // Check bounds
    if (x < 0 || x >= ParameterManager::GRID_SIZE || y < 0 || y >= ParameterManager::GRID_SIZE)
        return false;
        
    // Cell was active in previous grid but is now inactive
    return previousGrid[y][x] && !grid[y][x];
}

bool Grid::wasCellActive(int x, int y) const
{
    // Check bounds
    if (x < 0 || x >= ParameterManager::GRID_SIZE || y < 0 || y >= ParameterManager::GRID_SIZE)
        return false;
        
    // Return the state of the cell in the previous grid
    return previousGrid[y][x];
}

bool Grid::hasUpdated() const
{
    bool updated = gridHasUpdated;
    // We need to modify gridHasUpdated even in a const method
    // This is a logical const-ness situation where we're not changing the
    // observable state of the object, just an internal flag
    const_cast<Grid*>(this)->gridHasUpdated = false; // Reset the flag
    return updated;
}

} // namespace GameOfLifeApp
