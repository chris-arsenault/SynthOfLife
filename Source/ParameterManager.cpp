#include "ParameterManager.h"

ParameterManager::ParameterManager(juce::AudioProcessor& p)
    : processor(p),
      apvts(p, nullptr, "Parameters", createParameterLayout())
{
    initializeParameters();
}

ParameterManager::~ParameterManager()
{
    // Nothing to clean up
}

juce::AudioProcessorValueTreeState::ParameterLayout ParameterManager::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Default MIDI notes for samples (starting with GM drum map and extending)
    const int defaultMidiNotes[NUM_SAMPLES] = { 36, 38, 40, 41, 43, 45, 47, 49, 50, 51, 52, 53, 54, 55, 56, 57 };
    
    // Create output choices
    juce::StringArray outputChoices;
    outputChoices.add("Main Output"); // Output 0
    for (int i = 1; i < NUM_OUTPUTS; ++i)
    {
        outputChoices.add("Output " + juce::String(i)); // Outputs 1-16
    }
    
    // Create parameters for each sample
    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        // Volume parameter
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "volume_" + juce::String(i),
            "Sample " + juce::String(i + 1) + " Volume",
            0.0f, 1.0f, 0.8f));
            
        // Pan parameter
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "pan_" + juce::String(i),
            "Sample " + juce::String(i + 1) + " Pan",
            -1.0f, 1.0f, 0.0f));
            
        // Mute parameter
        layout.add(std::make_unique<juce::AudioParameterBool>(
            "mute_" + juce::String(i),
            "Sample " + juce::String(i + 1) + " Mute",
            false));
            
        // MIDI note parameter
        layout.add(std::make_unique<juce::AudioParameterInt>(
            "midi_note_" + juce::String(i),
            "Sample " + juce::String(i + 1) + " MIDI Note",
            0, 127, defaultMidiNotes[i]));
            
        // Polyphony parameter
        layout.add(std::make_unique<juce::AudioParameterInt>(
            "polyphony_" + juce::String(i),
            "Sample " + juce::String(i + 1) + " Polyphony",
            1, 16, 4)); // Default to 4 voices
            
        // Control mode parameters - now using separate boolean parameters for each mode
        layout.add(std::make_unique<juce::AudioParameterBool>(
            "velocity_mode_" + juce::String(i),
            "Sample " + juce::String(i + 1) + " Velocity Mode",
            true)); // Default to velocity mode enabled
            
        layout.add(std::make_unique<juce::AudioParameterBool>(
            "midi_pitch_" + juce::String(i),
            "Sample " + juce::String(i + 1) + " MIDI Pitch",
            false)); // Default to MIDI pitch disabled
            
        layout.add(std::make_unique<juce::AudioParameterBool>(
            "row_pitch_" + juce::String(i),
            "Sample " + juce::String(i + 1) + " Row Pitch",
            false)); // Default to row pitch disabled
            
        layout.add(std::make_unique<juce::AudioParameterBool>(
            "timing_mode_" + juce::String(i),
            "Sample " + juce::String(i + 1) + " Timing Mode",
            false)); // Default to timing mode disabled
            
        // Legato mode parameter
        layout.add(std::make_unique<juce::AudioParameterBool>(
            "legato_" + juce::String(i),
            "Sample " + juce::String(i + 1) + " Legato Mode",
            true)); // Default to true (current behavior)
            
        // ADSR parameters
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "attack_" + juce::String(i),
            "Sample " + juce::String(i + 1) + " Attack",
            0.1f, 2000.0f, 10.0f));
            
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "decay_" + juce::String(i),
            "Sample " + juce::String(i + 1) + " Decay",
            0.1f, 2000.0f, 100.0f));
            
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "sustain_" + juce::String(i),
            "Sample " + juce::String(i + 1) + " Sustain",
            0.0f, 1.0f, 0.7f));
            
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "release_" + juce::String(i),
            "Sample " + juce::String(i + 1) + " Release",
            0.1f, 2000.0f, 200.0f));
            
        // Output selection parameter
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "output_" + juce::String(i),
            "Sample " + juce::String(i + 1) + " Output",
            outputChoices,
            0)); // Default to main output (0)
    }
    
    // Interval parameters for Game of Life
    juce::StringArray intervalTypeChoices = { "Normal", "Dotted", "Triplet" };
    juce::StringArray intervalValueChoices = { "1/4 (Quarter)", "1/8 (Eighth)", "1/16 (Sixteenth)" };
    
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "intervalType",
        "Interval Type",
        intervalTypeChoices,
        0)); // Default to Normal
        
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "intervalValue",
        "Interval Value",
        intervalValueChoices,
        2)); // Default to 1/16th notes
    
    // Musical scale parameter
    juce::StringArray scaleChoices = { 
        "Major", 
        "Natural Minor", 
        "Harmonic Minor", 
        "Chromatic", 
        "Pentatonic", 
        "Blues" 
    };
    
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "musicalScale",
        "Musical Scale",
        scaleChoices,
        4));  // Default to Pentatonic
        
    // Root note parameter
    juce::StringArray noteNames;
    for (int i = 0; i < 12; ++i)
    {
        noteNames.add(juce::MidiMessage::getMidiNoteName(60 + i, true, true, 4));
    }
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "rootNote",
        "Root Note",
        noteNames,
        0));  // Default to C
        
    // Maximum timing delay parameter
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "maxTimingDelay",
        "Max Timing Delay",
        10.0f, 1000.0f, 160.0f));  // Range 10-1000ms (1 second), default 160ms
        
    // Add section iteration parameters
    for (int i = 0; i < 4; ++i)
    {
        // Section bars parameter
        layout.add(std::make_unique<juce::AudioParameterInt>(
            "section_bars_" + juce::String(i),
            "Section " + juce::String(i + 1) + " Bars",
            1, 16, 4));  // Range 1-16, default 4
            
        // Section grid state parameter
        layout.add(std::make_unique<juce::AudioParameterInt>(
            "section_grid_state_" + juce::String(i),
            "Section " + juce::String(i + 1) + " Grid State",
            0, INT_MAX, 0));  // Range 0-INT_MAX, default 0
            
        // Section randomize parameter
        layout.add(std::make_unique<juce::AudioParameterBool>(
            "section_randomize_" + juce::String(i),
            "Section " + juce::String(i + 1) + " Randomize",
            true));  // Default to true
            
        // Section density parameter
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "section_density_" + juce::String(i),
            "Section " + juce::String(i + 1) + " Density",
            0.1f, 0.9f, 0.5f));  // Range 0.1-0.9, default 0.5
    }
    
    return layout;
}

void ParameterManager::initializeParameters()
{
    // Initialize parameter pointers
    volumeParams.resize(NUM_SAMPLES);
    panParams.resize(NUM_SAMPLES);
    muteParams.resize(NUM_SAMPLES);
    midiNoteParams.resize(NUM_SAMPLES);
    polyphonyParams.resize(NUM_SAMPLES);
    velocityModeParams.resize(NUM_SAMPLES);
    midiPitchParams.resize(NUM_SAMPLES);
    rowPitchParams.resize(NUM_SAMPLES);
    timingModeParams.resize(NUM_SAMPLES);
    legatoParams.resize(NUM_SAMPLES);
    attackParams.resize(NUM_SAMPLES);
    decayParams.resize(NUM_SAMPLES);
    sustainParams.resize(NUM_SAMPLES);
    releaseParams.resize(NUM_SAMPLES);
    outputParams.resize(NUM_SAMPLES);
    
    // Get parameter pointers
    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        volumeParams[i] = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("volume_" + juce::String(i)));
        panParams[i] = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("pan_" + juce::String(i)));
        muteParams[i] = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("mute_" + juce::String(i)));
        midiNoteParams[i] = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter("midi_note_" + juce::String(i)));
        polyphonyParams[i] = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter("polyphony_" + juce::String(i)));
        velocityModeParams[i] = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("velocity_mode_" + juce::String(i)));
        midiPitchParams[i] = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("midi_pitch_" + juce::String(i)));
        rowPitchParams[i] = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("row_pitch_" + juce::String(i)));
        timingModeParams[i] = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("timing_mode_" + juce::String(i)));
        legatoParams[i] = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("legato_" + juce::String(i)));
        attackParams[i] = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("attack_" + juce::String(i)));
        decayParams[i] = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("decay_" + juce::String(i)));
        sustainParams[i] = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("sustain_" + juce::String(i)));
        releaseParams[i] = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("release_" + juce::String(i)));
        outputParams[i] = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("output_" + juce::String(i)));
    }
    
    // Get global parameter pointers
    intervalTypeParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("intervalType"));
    intervalValueParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("intervalValue"));
    musicalScaleParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("musicalScale"));
    rootNoteParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("rootNote"));
    maxTimingDelayParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("maxTimingDelay"));
    
    // Get section iteration parameter pointers
    for (int i = 0; i < 4; ++i)
    {
        sectionBarsParams[i] = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter("section_bars_" + juce::String(i)));
        sectionGridStateParams[i] = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter("section_grid_state_" + juce::String(i)));
        sectionRandomizeParams[i] = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("section_randomize_" + juce::String(i)));
        sectionDensityParams[i] = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("section_density_" + juce::String(i)));
    }
}

juce::AudioParameterFloat* ParameterManager::getVolumeParam(int sampleIndex)
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES)
        return volumeParams[sampleIndex];
        
    return nullptr;
}

juce::AudioParameterFloat* ParameterManager::getPanParam(int sampleIndex)
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES)
        return panParams[sampleIndex];
        
    return nullptr;
}

juce::AudioParameterBool* ParameterManager::getMuteParam(int sampleIndex)
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES)
        return muteParams[sampleIndex];
        
    return nullptr;
}

juce::AudioParameterInt* ParameterManager::getMidiNoteParam(int sampleIndex)
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES)
        return midiNoteParams[sampleIndex];
        
    return nullptr;
}

juce::AudioParameterInt* ParameterManager::getPolyphonyParam(int sampleIndex)
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES)
        return polyphonyParams[sampleIndex];
        
    return nullptr;
}

juce::AudioParameterBool* ParameterManager::getVelocityModeParam(int sampleIndex)
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES)
        return velocityModeParams[sampleIndex];
        
    return nullptr;
}

juce::AudioParameterBool* ParameterManager::getMidiPitchParam(int sampleIndex)
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES)
        return midiPitchParams[sampleIndex];
        
    return nullptr;
}

juce::AudioParameterBool* ParameterManager::getRowPitchParam(int sampleIndex)
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES)
        return rowPitchParams[sampleIndex];
        
    return nullptr;
}

juce::AudioParameterBool* ParameterManager::getTimingModeParam(int sampleIndex)
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES)
        return timingModeParams[sampleIndex];
        
    return nullptr;
}

juce::AudioParameterBool* ParameterManager::getLegatoParam(int sampleIndex)
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES)
        return legatoParams[sampleIndex];
        
    return nullptr;
}

juce::AudioParameterFloat* ParameterManager::getAttackParam(int sampleIndex)
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES)
        return attackParams[sampleIndex];
        
    return nullptr;
}

juce::AudioParameterFloat* ParameterManager::getDecayParam(int sampleIndex)
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES)
        return decayParams[sampleIndex];
        
    return nullptr;
}

juce::AudioParameterFloat* ParameterManager::getSustainParam(int sampleIndex)
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES)
        return sustainParams[sampleIndex];
        
    return nullptr;
}

juce::AudioParameterFloat* ParameterManager::getReleaseParam(int sampleIndex)
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES)
        return releaseParams[sampleIndex];
        
    return nullptr;
}

juce::AudioParameterChoice* ParameterManager::getOutputParam(int sampleIndex)
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES)
        return outputParams[sampleIndex];
        
    return nullptr;
}

juce::AudioParameterChoice* ParameterManager::getIntervalTypeParam()
{
    return intervalTypeParam;
}

juce::AudioParameterChoice* ParameterManager::getIntervalValueParam()
{
    return intervalValueParam;
}

juce::AudioParameterChoice* ParameterManager::getScaleParam()
{
    return musicalScaleParam;
}

juce::AudioParameterChoice* ParameterManager::getRootNoteParam()
{
    return rootNoteParam;
}

juce::AudioParameterFloat* ParameterManager::getMaxTimingDelayParam()
{
    return maxTimingDelayParam;
}

juce::AudioParameterInt* ParameterManager::getSectionBarsParam(int sectionIndex)
{
    if (sectionIndex >= 0 && sectionIndex < 4)
        return sectionBarsParams[sectionIndex];
        
    return nullptr;
}

juce::AudioParameterInt* ParameterManager::getSectionGridStateParam(int sectionIndex)
{
    if (sectionIndex >= 0 && sectionIndex < 4)
        return sectionGridStateParams[sectionIndex];
        
    return nullptr;
}

juce::AudioParameterBool* ParameterManager::getSectionRandomizeParam(int sectionIndex)
{
    if (sectionIndex >= 0 && sectionIndex < 4)
        return sectionRandomizeParams[sectionIndex];
        
    return nullptr;
}

juce::AudioParameterFloat* ParameterManager::getSectionDensityParam(int sectionIndex)
{
    if (sectionIndex >= 0 && sectionIndex < 4)
        return sectionDensityParams[sectionIndex];
        
    return nullptr;
}

MusicalScale ParameterManager::getSelectedScale() const
{
    if (musicalScaleParam != nullptr)
    {
        return static_cast<MusicalScale>(musicalScaleParam->getIndex());
    }
    
    return MusicalScale::Pentatonic; // Default to pentatonic
}

int ParameterManager::getSampleForColumn(int column) const
{
    // Direct 1:1 mapping between columns and samples
    if (column >= 0 && column < NUM_SAMPLES)
        return column;
        
    return -1; // Invalid column
}

ColumnControlMode ParameterManager::getControlModeForColumn(int column) const
{
    // Get the sample index for this column
    int sampleIndex = getSampleForColumn(column);
    
    // If the column is valid, get the control mode for the sample
    if (sampleIndex >= 0)
        return getControlModeForSample(sampleIndex);
        
    return ColumnControlMode::None;
}

ColumnControlMode ParameterManager::getControlModeForSample(int sampleIndex) const
{
    ColumnControlMode mode = ColumnControlMode::None;
    
    // Check if the sample index is valid
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES)
    {
        // Check if velocity mode is enabled
        if (velocityModeParams[sampleIndex] != nullptr && velocityModeParams[sampleIndex]->get())
        {
            mode = static_cast<ColumnControlMode>(static_cast<int>(mode) | static_cast<int>(ColumnControlMode::Velocity));
        }
        
        // Check if timing mode is enabled
        if (timingModeParams[sampleIndex] != nullptr && timingModeParams[sampleIndex]->get())
        {
            mode = static_cast<ColumnControlMode>(static_cast<int>(mode) | static_cast<int>(ColumnControlMode::Timing));
        }
    }
    
    return mode;
}

bool ParameterManager::getLegatoForSample(int sampleIndex) const
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES && legatoParams[sampleIndex] != nullptr)
    {
        return legatoParams[sampleIndex]->get();
    }
    
    return true; // Default to true
}

float ParameterManager::getVolumeForSample(int sampleIndex) const
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES && volumeParams[sampleIndex] != nullptr)
    {
        return volumeParams[sampleIndex]->get();
    }
    
    return 0.8f; // Default volume
}

float ParameterManager::getPanForSample(int sampleIndex) const
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES && panParams[sampleIndex] != nullptr)
    {
        return panParams[sampleIndex]->get();
    }
    
    return 0.0f; // Default pan (center)
}

bool ParameterManager::getMuteForSample(int sampleIndex) const
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES && muteParams[sampleIndex] != nullptr)
    {
        return muteParams[sampleIndex]->get();
    }
    
    return false; // Default to not muted
}

int ParameterManager::getMidiNoteForSample(int sampleIndex) const
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES && midiNoteParams[sampleIndex] != nullptr)
    {
        return midiNoteParams[sampleIndex]->get();
    }
    
    return 60; // Default to middle C
}

int ParameterManager::getPolyphonyForSample(int sampleIndex) const
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES && polyphonyParams[sampleIndex] != nullptr)
    {
        return polyphonyParams[sampleIndex]->get();
    }
    
    return 4; // Default to 4 voices
}

float ParameterManager::getAttackForSample(int sampleIndex) const
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES && attackParams[sampleIndex] != nullptr)
    {
        return attackParams[sampleIndex]->get();
    }
    
    return 10.0f; // Default attack time
}

float ParameterManager::getDecayForSample(int sampleIndex) const
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES && decayParams[sampleIndex] != nullptr)
    {
        return decayParams[sampleIndex]->get();
    }
    
    return 100.0f; // Default decay time
}

float ParameterManager::getSustainForSample(int sampleIndex) const
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES && sustainParams[sampleIndex] != nullptr)
    {
        return sustainParams[sampleIndex]->get();
    }
    
    return 0.7f; // Default sustain level
}

float ParameterManager::getReleaseForSample(int sampleIndex) const
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES && releaseParams[sampleIndex] != nullptr)
    {
        return releaseParams[sampleIndex]->get();
    }
    
    return 200.0f; // Default release time
}

int ParameterManager::getOutputForSample(int sampleIndex) const
{
    if (sampleIndex >= 0 && sampleIndex < NUM_SAMPLES && outputParams[sampleIndex] != nullptr)
    {
        return outputParams[sampleIndex]->getIndex();
    }
    
    return 0; // Default to main output
}

int ParameterManager::getPitchOffsetForRow(int row) const
{
    // Invert row index (0 is top row in the grid, but we want the bottom row to be the lowest pitch)
    int invertedRow = GRID_SIZE - 1 - row;
    
    // Get the selected scale
    MusicalScale scale = getSelectedScale();
    
    // Get the scale pattern
    const std::vector<int>& scalePattern = getScalePattern(scale);
    
    // Calculate the pitch offset
    int octave = invertedRow / scalePattern.size();
    int noteInScale = invertedRow % scalePattern.size();
    
    // For chromatic scale, just return the row index
    if (scale == MusicalScale::Chromatic)
    {
        return invertedRow - 7; // Center around middle row
    }
    
    // For other scales, map to the scale pattern
    return (octave * 12) + scalePattern[noteInScale] - 7; // Center around middle row
}

float ParameterManager::getTimingDelayForRow(int row) const
{
    // Get the maximum timing delay
    float maxDelay = maxTimingDelayParam != nullptr ? maxTimingDelayParam->get() : 160.0f;
    
    // Calculate the delay based on the row (0 is top row, GRID_SIZE-1 is bottom row)
    // Map row to a value between 0 and 1
    float normalizedRow = static_cast<float>(row) / static_cast<float>(GRID_SIZE - 1);
    
    // Map to delay (0 to maxDelay)
    return normalizedRow * maxDelay;
}

const std::vector<int>& ParameterManager::getScalePattern(MusicalScale scale) const
{
    // Define scale patterns (semitone intervals from the root note)
    static const std::vector<int> majorScale = { 0, 2, 4, 5, 7, 9, 11 };
    static const std::vector<int> naturalMinorScale = { 0, 2, 3, 5, 7, 8, 10 };
    static const std::vector<int> harmonicMinorScale = { 0, 2, 3, 5, 7, 8, 11 };
    static const std::vector<int> chromaticScale = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    static const std::vector<int> pentatonicScale = { 0, 2, 4, 7, 9 };
    static const std::vector<int> bluesScale = { 0, 3, 5, 6, 7, 10 };
    
    // Return the appropriate scale pattern
    switch (scale)
    {
        case MusicalScale::Major:
            return majorScale;
        case MusicalScale::NaturalMinor:
            return naturalMinorScale;
        case MusicalScale::HarmonicMinor:
            return harmonicMinorScale;
        case MusicalScale::Chromatic:
            return chromaticScale;
        case MusicalScale::Pentatonic:
            return pentatonicScale;
        case MusicalScale::Blues:
            return bluesScale;
        default:
            return pentatonicScale; // Default to pentatonic
    }
}
