#pragma once

#include <JuceHeader.h>
#include "../DrumPad.h"
#include "../ParameterManager.h"

class DrumPadComponent : public juce::Component,
                         public juce::Button::Listener,
                         public juce::Timer
{
public:
    DrumPadComponent(ParameterManager& paramManager);
    ~DrumPadComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Button listener
    void buttonClicked(juce::Button* button) override;
    
    // Timer callback
    void timerCallback() override;
    
    // Set the drum pads
    void setDrumPads(DrumPad* pads) { drumPads = pads; }
    
private:
    ParameterManager& paramManager;
    DrumPad* drumPads = nullptr;
    
    struct PadInfo
    {
        bool isPlaying = false;
        float lastVelocity = 0.0f;
        int lastPitch = 0;
        juce::Colour padColor = juce::Colours::darkgrey;
        juce::String statusText;
        
        std::unique_ptr<juce::TextButton> padButton;
        std::unique_ptr<juce::Label> infoLabel;
        
        // Volume history for visualization
        static constexpr int maxHistorySize = 120; // 4 seconds at 30 fps
        std::array<float, maxHistorySize> volumeHistory;
        int historyIndex = 0;
        bool historyFilled = false;
        
        // Reset the volume history
        void resetVolumeHistory()
        {
            std::fill(volumeHistory.begin(), volumeHistory.end(), 0.0f);
            historyIndex = 0;
            historyFilled = false;
        }
        
        // Add a volume sample to the history
        void addVolumeToHistory(float volume)
        {
            volumeHistory[historyIndex] = volume;
            historyIndex = (historyIndex + 1) % maxHistorySize;
            if (historyIndex == 0)
                historyFilled = true;
        }
    };
    
    std::array<PadInfo, ParameterManager::NUM_SAMPLES> pads;
    
    // Helper method to update pad information
    void updatePadInfo();
    
    // Helper method to get color based on velocity and pitch
    juce::Colour getColorForPad(float velocity, int pitch);
    
    // Helper method to draw the volume graph
    void drawVolumeGraph(juce::Graphics& g, const PadInfo& pad, juce::Rectangle<int> bounds);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumPadComponent)
};
