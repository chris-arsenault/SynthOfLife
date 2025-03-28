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
    
    // Default MIDI notes for drum pads (starting with GM drum map)
    const int defaultMidiNotes[NUM_DRUM_PADS] = { 36, 38, 40, 41, 43, 45, 47, 49 };
    
    // Create parameters for each drum pad
    for (int i = 0; i < NUM_DRUM_PADS; ++i)
    {
        // Volume parameter
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "volume" + juce::String(i),
            "Pad " + juce::String(i + 1) + " Volume",
            0.0f, 1.0f, 0.8f));
            
        // Pan parameter
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "pan" + juce::String(i),
            "Pad " + juce::String(i + 1) + " Pan",
            -1.0f, 1.0f, 0.0f));
            
        // Mute parameter
        layout.add(std::make_unique<juce::AudioParameterBool>(
            "mute" + juce::String(i),
            "Pad " + juce::String(i + 1) + " Mute",
            false));
            
        // MIDI note parameter
        layout.add(std::make_unique<juce::AudioParameterInt>(
            "note" + juce::String(i),
            "Pad " + juce::String(i + 1) + " MIDI Note",
            0, 127, defaultMidiNotes[i]));
            
        // Polyphony parameter
        layout.add(std::make_unique<juce::AudioParameterInt>(
            "poly" + juce::String(i),
            "Pad " + juce::String(i + 1) + " Polyphony",
            1, 16, 4)); // Default to 4 voices
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
    
    // Game of Life parameters
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "golEnabled",
        "Game of Life Enabled",
        false));
        
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "golRandomize",
        "Game of Life Randomize",
        false));
        
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
        0)); // Default to Major scale
        
    // Column mapping parameters
    for (int col = 0; col < GRID_SIZE; ++col)
    {
        juce::StringArray choices;
        choices.add("None");
        
        for (int i = 0; i < NUM_DRUM_PADS; ++i)
        {
            choices.add("Pad " + juce::String(i + 1));
        }
        
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "colMap" + juce::String(col),
            "Column " + juce::String(col + 1) + " Mapping",
            choices,
            0)); // Default to "None"
            
        // Column control mode parameters
        juce::StringArray modeChoices;
        modeChoices.add("Velocity");
        modeChoices.add("Pitch");
        
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "colMode" + juce::String(col),
            "Column " + juce::String(col + 1) + " Mode",
            modeChoices,
            0)); // Default to "Velocity"
    }
    
    return layout;
}

void ParameterManager::initializeParameters()
{
    // Store parameter pointers for easy access
    for (int i = 0; i < NUM_DRUM_PADS; ++i)
    {
        volumeParams[i] = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("volume" + juce::String(i)));
        panParams[i] = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("pan" + juce::String(i)));
        muteParams[i] = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("mute" + juce::String(i)));
        midiNoteParams[i] = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter("note" + juce::String(i)));
        polyphonyParams[i] = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter("poly" + juce::String(i)));
    }
    
    intervalTypeParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("intervalType"));
    intervalValueParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("intervalValue"));
    
    gameOfLifeEnabledParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("golEnabled"));
    gameOfLifeRandomizeParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("golRandomize"));
    
    musicalScaleParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("musicalScale"));
    
    for (int col = 0; col < GRID_SIZE; ++col)
    {
        columnMappingParams[col] = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("colMap" + juce::String(col)));
        columnControlModeParams[col] = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("colMode" + juce::String(col)));
    }
}

juce::AudioParameterFloat* ParameterManager::getVolumeParam(int padIndex)
{
    if (padIndex >= 0 && padIndex < NUM_DRUM_PADS)
        return volumeParams[padIndex];
    return nullptr;
}

juce::AudioParameterFloat* ParameterManager::getPanParam(int padIndex)
{
    if (padIndex >= 0 && padIndex < NUM_DRUM_PADS)
        return panParams[padIndex];
    return nullptr;
}

juce::AudioParameterBool* ParameterManager::getMuteParam(int padIndex)
{
    if (padIndex >= 0 && padIndex < NUM_DRUM_PADS)
        return muteParams[padIndex];
    return nullptr;
}

juce::AudioParameterInt* ParameterManager::getMidiNoteParam(int padIndex)
{
    if (padIndex >= 0 && padIndex < NUM_DRUM_PADS)
        return midiNoteParams[padIndex];
    return nullptr;
}

juce::AudioParameterInt* ParameterManager::getPolyphonyParam(int padIndex)
{
    if (padIndex >= 0 && padIndex < NUM_DRUM_PADS)
        return polyphonyParams[padIndex];
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

juce::AudioParameterBool* ParameterManager::getGameOfLifeEnabledParam()
{
    return gameOfLifeEnabledParam;
}

juce::AudioParameterBool* ParameterManager::getGameOfLifeRandomizeParam()
{
    return gameOfLifeRandomizeParam;
}

MusicalScale ParameterManager::getSelectedScale() const
{
    if (musicalScaleParam != nullptr)
    {
        return static_cast<MusicalScale>(musicalScaleParam->getIndex());
    }
    
    return MusicalScale::Major; // Default to Major scale
}

int ParameterManager::getSampleForColumn(int column) const
{
    if (column >= 0 && column < GRID_SIZE)
    {
        int mappingIndex = columnMappingParams[column]->getIndex();
        
        // Index 0 is "None"
        if (mappingIndex == 0)
            return -1;
            
        // Otherwise, it's (index - 1) because we added "None" at the beginning
        return mappingIndex - 1;
    }
    
    return -1; // Invalid column
}

ColumnControlMode ParameterManager::getControlModeForColumn(int column) const
{
    if (column >= 0 && column < GRID_SIZE)
    {
        int modeIndex = columnControlModeParams[column]->getIndex();
        return static_cast<ColumnControlMode>(modeIndex);
    }
    
    return ColumnControlMode::Velocity; // Default to velocity
}
