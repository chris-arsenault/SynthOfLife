#pragma once

#include <JuceHeader.h>
#include "../GameOfLife.h"
#include "../ParameterManager.h"

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
    
    // Set the GameOfLife model
    void setGameOfLife(GameOfLife* model) { gameOfLife = model; }
    
private:
    ParameterManager& paramManager;
    GameOfLife* gameOfLife = nullptr;
    
    juce::Label midiControlLabel;
    juce::TextButton randomizeButton;
    
    // Interval controls
    juce::Label intervalLabel;
    juce::ComboBox intervalTypeBox;
    juce::ComboBox intervalValueBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> intervalTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> intervalValueAttachment;
    
    // Column mapping controls
    juce::Label mappingLabel;
    std::array<std::unique_ptr<juce::ComboBox>, ParameterManager::GRID_SIZE> columnMappers;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>, ParameterManager::GRID_SIZE> columnMapperAttachments;
    
    // Column control mode controls
    juce::Label modeLabel;
    juce::Label pitchRangeLabel;
    std::array<std::unique_ptr<juce::ComboBox>, ParameterManager::GRID_SIZE> columnControlModes;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>, ParameterManager::GRID_SIZE> columnControlModeAttachments;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> randomizeAttachment;
    
    // Convert mouse position to grid coordinates
    bool getCellCoordinates(const juce::Point<int>& position, int& x, int& y);
    
    // Draw a single cell
    void drawCell(juce::Graphics& g, int x, int y, bool alive, int cellSize);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GameOfLifeComponent)
};
