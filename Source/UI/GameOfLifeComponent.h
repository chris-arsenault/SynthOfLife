#pragma once

#include <JuceHeader.h>
#include "../ParameterManager.h"
#include "../GameOfLife.h"
#include "../PluginProcessor.h"

/**
 * Component for displaying and interacting with the Game of Life grid
 */
class GameOfLifeComponent : public juce::Component,
                             public juce::Button::Listener,
                             public juce::Timer,
                             public juce::ComboBox::Listener
{
public:
    GameOfLifeComponent(ParameterManager& paramManager);
    ~GameOfLifeComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    
    // Button listener
    void buttonClicked(juce::Button* button) override;
    
    // ComboBox listener
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    
    // Timer callback
    void timerCallback() override;
    
    // Set the GameOfLife instance
    void setGameOfLife(GameOfLife* newGameOfLife) { gameOfLife = newGameOfLife; }
    
    // Get the GameOfLife instance
    GameOfLife* getGameOfLife() const { return gameOfLife; }
    
    // Update UI elements
    void updateUI() { repaint(); }
    
    // Get the grid state as a string (make it public so it can be accessed from PluginEditor)
    juce::String getGridStateAsString() const;
    
    // Set grid state from string representation (make it public so it can be accessed from PluginEditor)
    void setGridStateFromString(const juce::String& stateString);
    
    // Update the grid display
    void updateGrid() { repaint(); }
    
private:
    ParameterManager& paramManager;
    GameOfLife* gameOfLife = nullptr;
    
    juce::Label midiControlLabel;
    juce::TextButton randomizeButton;
    juce::TextButton clearButton;
    juce::TextEditor gridStateTextBox;
    juce::Label gridStateLabel;
    
    // Interval controls - Removed as they've been moved to the main tab
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> randomizeAttachment;
    
    // Convert mouse position to grid coordinates
    bool getCellCoordinates(const juce::Point<int>& position, int& x, int& y);
    
    // Draw a single cell
    void drawCell(juce::Graphics& g, int x, int y, bool alive, int cellSize);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GameOfLifeComponent)
};
