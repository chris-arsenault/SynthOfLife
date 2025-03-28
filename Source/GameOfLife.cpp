#include "GameOfLife.h"

GameOfLife::GameOfLife() : gridHasUpdated(false)
{
    // Initialize all grids to empty
    for (int y = 0; y < ParameterManager::GRID_SIZE; ++y)
    {
        for (int x = 0; x < ParameterManager::GRID_SIZE; ++x)
        {
            grid[y][x] = false;
            nextGrid[y][x] = false;
            previousGrid[y][x] = false;
        }
    }
}

GameOfLife::~GameOfLife()
{
    // Nothing to clean up
}

void GameOfLife::initialize(bool randomize)
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

void GameOfLife::update()
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
            bool currentState = grid[y][x];
            
            // Apply Conway's Game of Life rules
            if (currentState)
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
    
    // Update current grid with next state
    for (int y = 0; y < ParameterManager::GRID_SIZE; ++y)
    {
        for (int x = 0; x < ParameterManager::GRID_SIZE; ++x)
        {
            grid[y][x] = nextGrid[y][x];
        }
    }
    
    // Set the updated flag to true
    gridHasUpdated = true;
}

int GameOfLife::countLiveNeighbors(int x, int y) const
{
    int liveNeighbors = 0;
    
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
            
            if (grid[ny][nx])
                liveNeighbors++;
        }
    }
    
    return liveNeighbors;
}

bool GameOfLife::getCellState(int x, int y) const
{
    if (x >= 0 && x < ParameterManager::GRID_SIZE && y >= 0 && y < ParameterManager::GRID_SIZE)
        return grid[y][x];
    
    return false;
}

void GameOfLife::setCellState(int x, int y, bool state)
{
    if (x >= 0 && x < ParameterManager::GRID_SIZE && y >= 0 && y < ParameterManager::GRID_SIZE)
    {
        grid[y][x] = state;
        gridHasUpdated = true;
    }
}

void GameOfLife::toggleCellState(int x, int y)
{
    if (x >= 0 && x < ParameterManager::GRID_SIZE && y >= 0 && y < ParameterManager::GRID_SIZE)
    {
        grid[y][x] = !grid[y][x];
        gridHasUpdated = true;
    }
}
