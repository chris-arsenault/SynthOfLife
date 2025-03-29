#pragma once

#include <JuceHeader.h>
#include "DrumPad.h"
#include "GameOfLife.h"
#include "ParameterManager.h"
#include "UI/NoteActivityIndicator.h"

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
    
    // Process MIDI messages
    void processMidiMessages(juce::MidiBuffer& midiMessages);
    
    // Process Game of Life
    void processGameOfLife();
    
    // Check if any MIDI note is active
    bool isAnyNoteActive() const { return !activeNotes.empty(); }
    
    // Calculate interval in MIDI clock ticks
    int calculateIntervalInTicks();
    
    // Get the Game of Life instance
    GameOfLife& getGameOfLife() { return *gameOfLife.get(); }
    
    // Get the Parameter Manager instance
    ParameterManager& getParameterManager() { return *parameterManager.get(); }
    
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
    
    // Set the audio visualizer component
    void setAudioVisualizer(juce::AudioVisualiserComponent* visualizer) { audioVisualizer = visualizer; }
    
    // Set the note activity indicator component
    void setNoteActivityIndicator(NoteActivityIndicator* indicator) { noteActivityIndicator = indicator; }
    
    // Notify listeners that the state has been loaded
    void notifyStateLoaded();
    
    // Add a listener for state loading
    class StateLoadedListener
    {
    public:
        virtual ~StateLoadedListener() = default;
        virtual void stateLoaded() = 0;
    };
    
    void addStateLoadedListener(StateLoadedListener* listener);
    void removeStateLoadedListener(StateLoadedListener* listener);
    
    // Drum pads (one for each sample)
    std::array<DrumPad, ParameterManager::NUM_SAMPLES> drumPads;
    
    // Interval types and values
    enum class IntervalType { Normal, Dotted, Triplet };
    enum class IntervalValue { Quarter, Eighth, Sixteenth };

    // Output bus constants
    static constexpr int MAIN_OUTPUT_BUS = 0;
    static constexpr int NUM_ADDITIONAL_OUTPUTS = 16;
    static constexpr int TOTAL_OUTPUT_BUSES = 17; // Main output + 16 additional outputs

private:
    // Constants
    static constexpr int MIDDLE_C = 60; // MIDI note number for middle C
    
    // Parameter manager
    std::unique_ptr<ParameterManager> parameterManager;
    
    // Game of Life
    std::unique_ptr<GameOfLife> gameOfLife;
    
    // Scheduled sample data structure
    struct ScheduledSample
    {
        int sampleIndex;
        float velocity;
        int pitchShift;
        int cellX;
        int cellY;
        double triggerTime;
        bool active;
        
        ScheduledSample(int index, float vel, int pitch, int x, int y, double time)
            : sampleIndex(index), velocity(vel), pitchShift(pitch), 
              cellX(x), cellY(y), triggerTime(time), active(true) {}
    };
    
    // Queue of samples scheduled to be triggered with delay
    std::vector<ScheduledSample> scheduledSamples;
    
    // Schedule a sample to be triggered with a delay
    void scheduleSampleWithDelay(int sampleIndex, float velocity, int pitchShift, 
                                int cellX, int cellY, float delayMs);
    
    // Process any scheduled samples that are due to be triggered
    void processScheduledSamples(double currentTime);
    
    // MIDI clock counter
    int midiClockCounter = 0;
    
    // MIDI clock enabled flag
    bool midiClockEnabled = false;
    
    // Game of Life last update time
    double lastGameOfLifeUpdateTime = 0.0;
    
    // Game of Life MIDI control state
    bool gameOfLifeEnabled = false;
    
    // MIDI note tracking
    std::set<int> activeNotes;
    int mostRecentMidiNote = MIDDLE_C;
    
    // Current BPM
    double currentBPM = 120.0;
    
    // Visualization buffer
    juce::AudioBuffer<float> visualizationBuffer { 1, 1024 };
    
    // Audio visualizer component
    juce::AudioVisualiserComponent* audioVisualizer = nullptr;
    
    // Note activity indicator component
    NoteActivityIndicator* noteActivityIndicator = nullptr;

    // State loaded listeners
    std::vector<StateLoadedListener*> stateLoadedListeners;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumMachineAudioProcessor)
};