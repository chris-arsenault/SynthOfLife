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
    static const int NUM_DRUM_PADS = 8;
    static const int GRID_SIZE = 16; // Added to replace GameOfLife::GRID_SIZE
    
    ParameterManager(juce::AudioProcessor& processor);
    ~ParameterManager();
    
    // Create parameter layout
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Initialize parameters
    void initializeParameters();
    
    // Get parameter pointers
    juce::AudioParameterFloat* getVolumeParam(int padIndex);
    juce::AudioParameterFloat* getPanParam(int padIndex);
    juce::AudioParameterBool* getMuteParam(int padIndex);
    juce::AudioParameterInt* getMidiNoteParam(int padIndex);
    juce::AudioParameterInt* getPolyphonyParam(int padIndex);
    
    juce::AudioParameterChoice* getIntervalTypeParam();
    juce::AudioParameterChoice* getIntervalValueParam();
    
    juce::AudioParameterBool* getGameOfLifeEnabledParam();
    juce::AudioParameterBool* getGameOfLifeRandomizeParam();
    
    // Get column mapping and mode parameters
    int getSampleForColumn(int column) const;
    ColumnControlMode getControlModeForColumn(int column) const;
    
    // Get the selected musical scale
    MusicalScale getSelectedScale() const;
    
    // Get the AudioProcessorValueTreeState
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    
private:
    juce::AudioProcessor& processor;
    juce::AudioProcessorValueTreeState apvts;
    
    // Parameter pointers
    juce::AudioParameterFloat* volumeParams[NUM_DRUM_PADS];
    juce::AudioParameterFloat* panParams[NUM_DRUM_PADS];
    juce::AudioParameterBool* muteParams[NUM_DRUM_PADS];
    juce::AudioParameterInt* midiNoteParams[NUM_DRUM_PADS];
    juce::AudioParameterInt* polyphonyParams[NUM_DRUM_PADS];
    
    juce::AudioParameterChoice* intervalTypeParam;
    juce::AudioParameterChoice* intervalValueParam;
    
    juce::AudioParameterBool* gameOfLifeEnabledParam;
    juce::AudioParameterBool* gameOfLifeRandomizeParam;
    juce::AudioParameterChoice* columnMappingParams[GRID_SIZE];
    juce::AudioParameterChoice* columnControlModeParams[GRID_SIZE];
    juce::AudioParameterChoice* musicalScaleParam;
};
