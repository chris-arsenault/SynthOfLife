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

// Column control modes - now using bit flags for multiple selections
enum class ColumnControlMode
{
    None = 0,
    Velocity = 1,
    Pitch = 2,
    Timing = 4,
    Both = Velocity | Pitch,
    All = Velocity | Pitch | Timing
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
    juce::AudioParameterBool* getVelocityModeParam(int sampleIndex);
    juce::AudioParameterBool* getPitchModeParam(int sampleIndex);
    juce::AudioParameterBool* getTimingModeParam(int sampleIndex);
    juce::AudioParameterBool* getLegatoParam(int sampleIndex);
    juce::AudioParameterFloat* getAttackParam(int sampleIndex);
    juce::AudioParameterFloat* getDecayParam(int sampleIndex);
    juce::AudioParameterFloat* getSustainParam(int sampleIndex);
    juce::AudioParameterFloat* getReleaseParam(int sampleIndex);
    
    juce::AudioParameterChoice* getIntervalTypeParam();
    juce::AudioParameterChoice* getIntervalValueParam();
    juce::AudioParameterChoice* getScaleParam();
    juce::AudioParameterChoice* getRootNoteParam();
    juce::AudioParameterFloat* getMaxTimingDelayParam();
    
    // Section iteration parameters
    juce::AudioParameterInt* getSectionBarsParam(int sectionIndex);
    juce::AudioParameterInt* getSectionGridStateParam(int sectionIndex);
    juce::AudioParameterBool* getSectionRandomizeParam(int sectionIndex);
    juce::AudioParameterFloat* getSectionDensityParam(int sectionIndex);
    
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
    
    // Get the timing delay for a specific row (0-160ms)
    float getTimingDelayForRow(int row) const;
    
    // Get the scale pattern for a given scale
    const std::vector<int>& getScalePattern(MusicalScale scale) const;
    
    // Get the AudioProcessorValueTreeState
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    
    // Get the processor reference
    juce::AudioProcessor* getProcessor() { return &processor; }
    
private:
    juce::AudioProcessor& processor;
    juce::AudioProcessorValueTreeState apvts;
    
    // Parameter pointers
    std::vector<juce::AudioParameterFloat*> volumeParams;
    std::vector<juce::AudioParameterFloat*> panParams;
    std::vector<juce::AudioParameterBool*> muteParams;
    std::vector<juce::AudioParameterInt*> midiNoteParams;
    std::vector<juce::AudioParameterInt*> polyphonyParams;
    std::vector<juce::AudioParameterBool*> velocityModeParams;
    std::vector<juce::AudioParameterBool*> pitchModeParams;
    std::vector<juce::AudioParameterBool*> timingModeParams;
    std::vector<juce::AudioParameterBool*> legatoParams;
    std::vector<juce::AudioParameterFloat*> attackParams;
    std::vector<juce::AudioParameterFloat*> decayParams;
    std::vector<juce::AudioParameterFloat*> sustainParams;
    std::vector<juce::AudioParameterFloat*> releaseParams;
    
    juce::AudioParameterChoice* intervalTypeParam = nullptr;
    juce::AudioParameterChoice* intervalValueParam = nullptr;
    juce::AudioParameterChoice* musicalScaleParam = nullptr;
    juce::AudioParameterChoice* rootNoteParam = nullptr;
    juce::AudioParameterFloat* maxTimingDelayParam = nullptr;
    
    // Section iteration parameters
    juce::AudioParameterInt* sectionBarsParams[4] = { nullptr };
    juce::AudioParameterInt* sectionGridStateParams[4] = { nullptr };
    juce::AudioParameterBool* sectionRandomizeParams[4] = { nullptr };
    juce::AudioParameterFloat* sectionDensityParams[4] = { nullptr };
};
