#pragma once

#include <JuceHeader.h>

// Forward declaration
class GameOfLife;

// Note interval types
enum class IntervalType
{
    Normal = 0,
    Dotted,
    Triplet,
    NumIntervalTypes
};

// Note interval values
enum class IntervalValue
{
    Quarter = 0,
    Eighth,
    Sixteenth,
    NumIntervalValues
};

// Column control modes
enum class ColumnControlMode
{
    Velocity = 0,
    Pitch,
    NumControlModes
};

// Musical scales
enum class MusicalScale
{
    Major = 0,
    NaturalMinor,
    HarmonicMinor,
    Chromatic,
    Pentatonic,
    Blues,
    NumScales
};

class ParameterManager
{
public:
    static const int NUM_SAMPLES = 16; 
    static const int GRID_SIZE = 16;
    
    ParameterManager(juce::AudioProcessor& processor);
    ~ParameterManager();
    
    // Create parameter layout
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Initialize parameters
    void initializeParameters();
    
    // Get parameter pointers
    juce::AudioParameterFloat* getVolumeParam(int sampleIndex);
    juce::AudioParameterFloat* getPanParam(int sampleIndex);
    juce::AudioParameterBool* getMuteParam(int sampleIndex);
    juce::AudioParameterInt* getMidiNoteParam(int sampleIndex);
    juce::AudioParameterInt* getPolyphonyParam(int sampleIndex);
    juce::AudioParameterChoice* getControlModeParam(int sampleIndex);
    juce::AudioParameterBool* getLegatoParam(int sampleIndex);
    juce::AudioParameterFloat* getAttackParam(int sampleIndex);
    juce::AudioParameterFloat* getDecayParam(int sampleIndex);
    juce::AudioParameterFloat* getSustainParam(int sampleIndex);
    juce::AudioParameterFloat* getReleaseParam(int sampleIndex);
    
    juce::AudioParameterChoice* getIntervalTypeParam();
    juce::AudioParameterChoice* getIntervalValueParam();
    juce::AudioParameterChoice* getScaleParam();
    juce::AudioParameterChoice* getRootNoteParam();
    
    // Get sample for column (now direct 1:1 mapping)
    int getSampleForColumn(int column) const;
    
    // Get control mode for column (now maps to the sample's control mode)
    ColumnControlMode getControlModeForColumn(int column) const;
    
    // Get control mode for a specific sample
    ColumnControlMode getControlModeForSample(int sampleIndex) const;
    
    // Get volume for a specific sample
    float getVolumeForSample(int sampleIndex) const;
    
    // Get pan for a specific sample
    float getPanForSample(int sampleIndex) const;
    
    // Check if a sample is muted
    bool getMuteForSample(int sampleIndex) const;
    
    // Get MIDI note for a specific sample
    int getMidiNoteForSample(int sampleIndex) const;
    
    // Get polyphony for a specific sample
    int getPolyphonyForSample(int sampleIndex) const;
    
    // Check if a sample is in legato mode
    bool getLegatoForSample(int sampleIndex) const;
    
    // Get attack for a specific sample
    float getAttackForSample(int sampleIndex) const;
    
    // Get decay for a specific sample
    float getDecayForSample(int sampleIndex) const;
    
    // Get sustain for a specific sample
    float getSustainForSample(int sampleIndex) const;
    
    // Get release for a specific sample
    float getReleaseForSample(int sampleIndex) const;
    
    // Get the selected musical scale
    MusicalScale getSelectedScale() const;
    
    // Get pitch offset for a row based on the selected scale
    // Row 0 is the top row, row (GRID_SIZE-1) is the bottom row
    // Returns a semitone offset in the range -7 to +8
    int getPitchOffsetForRow(int row) const;
    
    // Get the AudioProcessorValueTreeState
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    
    // Get the processor reference
    juce::AudioProcessor* getProcessor() { return &processor; }
    
private:
    juce::AudioProcessor& processor;
    juce::AudioProcessorValueTreeState apvts;
    
    // Parameter pointers
    juce::AudioParameterFloat* volumeParams[NUM_SAMPLES];
    juce::AudioParameterFloat* panParams[NUM_SAMPLES];
    juce::AudioParameterBool* muteParams[NUM_SAMPLES];
    juce::AudioParameterInt* midiNoteParams[NUM_SAMPLES];
    juce::AudioParameterInt* polyphonyParams[NUM_SAMPLES];
    juce::AudioParameterChoice* controlModeParams[NUM_SAMPLES]; 
    
    juce::AudioParameterBool* legatoParams[NUM_SAMPLES];
    
    juce::AudioParameterFloat* attackParams[NUM_SAMPLES];
    juce::AudioParameterFloat* decayParams[NUM_SAMPLES];
    juce::AudioParameterFloat* sustainParams[NUM_SAMPLES];
    juce::AudioParameterFloat* releaseParams[NUM_SAMPLES];
    
    juce::AudioParameterChoice* intervalTypeParam;
    juce::AudioParameterChoice* intervalValueParam;
    
    juce::AudioParameterChoice* musicalScaleParam;
    juce::AudioParameterChoice* rootNoteParam;
};
