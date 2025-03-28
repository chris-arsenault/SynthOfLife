#pragma once

#include <JuceHeader.h>
#include "DrumPad.h"
#include "GameOfLife.h"
#include "ParameterManager.h"

//==============================================================================
/**
*/
class DrumMachineAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    DrumMachineAudioProcessor();
    ~DrumMachineAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    //==============================================================================
    // Custom methods
    
    // Trigger a sample
    void triggerSample(int padIndex, float velocity);
    
    // Trigger a sample with pitch shift
    void triggerSampleWithPitch(int padIndex, float velocity, int pitchShiftSemitones);
    
    // Process MIDI clock
    void processMidiClock();
    
    // Calculate interval in MIDI clock ticks
    int calculateIntervalInTicks();
    
    // Get the Game of Life instance
    GameOfLife& getGameOfLife() { return *gameOfLife.get(); }
    
    // Get the Parameter Manager instance
    ParameterManager& getParameterManager() { return *parameterManager.get(); }
    
    // Check if a note is currently active
    bool getNoteActiveStatus() const { return isNoteActive; }
    
    // Get the visualization buffer
    juce::AudioBuffer<float>& getVisualizationBuffer() { return visualizationBuffer; }
    
    // Get the waveform buffer for visualization
    juce::AudioBuffer<float> getWaveformBuffer() 
    { 
        // Create a copy of the visualization buffer for the UI to use
        juce::AudioBuffer<float> buffer(2, 256);
        buffer.clear();
        
        // If we have data in the visualization buffer, copy it
        if (visualizationBuffer.getNumSamples() > 0)
        {
            // Just copy the first channel to both channels of the output buffer
            for (int i = 0; i < buffer.getNumSamples() && i < visualizationBuffer.getNumSamples(); ++i)
            {
                buffer.setSample(0, i, visualizationBuffer.getSample(0, i));
                buffer.setSample(1, i, visualizationBuffer.getSample(0, i));
            }
        }
        
        return buffer;
    }
    
    // Drum pads array
    std::array<DrumPad, ParameterManager::NUM_DRUM_PADS> drumPads;
    
    // Interval types and values
    enum class IntervalType { Normal, Dotted, Triplet };
    enum class IntervalValue { Quarter, Eighth, Sixteenth };

private:
    // Parameter manager
    std::unique_ptr<ParameterManager> parameterManager;
    
    // Game of Life
    std::unique_ptr<GameOfLife> gameOfLife;
    
    // MIDI clock counter
    int midiClockCounter = 0;
    
    // Host playback state
    bool isHostPlaying = false;
    
    // Game of Life MIDI control state
    bool gameOfLifeEnabled = false;
    
    // Note active indicator
    bool isNoteActive = false;
    
    // Current BPM
    double currentBPM = 120.0;
    
    // Visualization buffer
    juce::AudioBuffer<float> visualizationBuffer { 1, 1024 };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumMachineAudioProcessor)
};
