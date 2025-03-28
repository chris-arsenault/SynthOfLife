#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DrumMachineAudioProcessor::DrumMachineAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    // Create parameter manager
    parameterManager = std::make_unique<ParameterManager>(*this);
    
    // Create Game of Life with reference to parameter manager
    gameOfLife = std::make_unique<GameOfLife>(parameterManager.get());
    
    // Initialize Game of Life with random cells
    gameOfLife->initialize(true);
    
    // Initialize visualization buffer
    visualizationBuffer.setSize(1, 1024);
    visualizationBuffer.clear();
    
    isNoteActive = false;
    mostRecentMidiNote = 60; // Initialize to C3 (60) instead of 0
    
    activeNotes = std::set<int>();
}

DrumMachineAudioProcessor::~DrumMachineAudioProcessor()
{
}

//==============================================================================
const juce::String DrumMachineAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DrumMachineAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DrumMachineAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DrumMachineAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DrumMachineAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DrumMachineAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DrumMachineAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DrumMachineAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DrumMachineAudioProcessor::getProgramName (int index)
{
    return {};
}

void DrumMachineAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DrumMachineAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Prepare all drum pads for playback
    for (int i = 0; i < ParameterManager::NUM_DRUM_PADS; ++i)
    {
        drumPads[i].prepareToPlay(sampleRate, samplesPerBlock);
        
        // Set the polyphony for each drum pad
        auto* polyphonyParam = parameterManager->getPolyphonyParam(i);
        if (polyphonyParam != nullptr)
        {
            drumPads[i].setPolyphony(polyphonyParam->get());
        }
        
        // Debug output for sample paths
        DBG("Pad " + juce::String(i) + " sample path: " + drumPads[i].getSamplePath());
    }
    
    // Initialize visualization buffer
    visualizationBuffer.setSize(1, samplesPerBlock);
    visualizationBuffer.clear();
    
    // Reset MIDI clock counter
    midiClockCounter = 0;
    
    // Debug output
    DBG("prepareToPlay called with sample rate: " + juce::String(sampleRate) + 
        ", samples per block: " + juce::String(samplesPerBlock));
}

void DrumMachineAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DrumMachineAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void DrumMachineAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear output buffer
    buffer.clear();
    
    // Update parameters for each drum pad
    for (int i = 0; i < ParameterManager::NUM_DRUM_PADS; ++i)
    {
        auto* volumeParam = parameterManager->getVolumeParam(i);
        auto* panParam = parameterManager->getPanParam(i);
        auto* muteParam = parameterManager->getMuteParam(i);
        auto* polyphonyParam = parameterManager->getPolyphonyParam(i);
        
        if (volumeParam != nullptr)
            drumPads[i].setVolume(volumeParam->get());
            
        if (panParam != nullptr)
            drumPads[i].setPan(panParam->get());
            
        if (muteParam != nullptr)
            drumPads[i].setMuted(muteParam->get());
            
        if (polyphonyParam != nullptr)
            drumPads[i].setPolyphony(polyphonyParam->get());
    }
    
    // Check if we have MIDI messages
    bool hasMidiMessages = !midiMessages.isEmpty();
    
    // Process MIDI messages
    juce::MidiBuffer::Iterator it(midiMessages);
    juce::MidiMessage message;
    int samplePosition;

    while (it.getNextEvent(message, samplePosition))
    {
        // Handle MIDI note on messages
        if (message.isNoteOn())
        {
            int note = message.getNoteNumber();
            float velocity = message.getVelocity() / 127.0f;
            
            // Store the most recent MIDI note for pitch control
            mostRecentMidiNote = note;
            
            // Check if this is the first note being pressed
            bool isFirstNote = activeNotes.empty();
            
            // Add note to active notes set
            activeNotes.insert(note);
            
            // Enable Game of Life when any note is pressed
            gameOfLifeEnabled = true;
            isNoteActive = true;
            
            // Debug output
            DBG("MIDI Note On: " + juce::String(note) + 
                ", velocity: " + juce::String(velocity) + 
                ", Game of Life enabled: " + juce::String(gameOfLifeEnabled ? "yes" : "no") +
                ", Active notes: " + juce::String(activeNotes.size()) +
                ", First note: " + juce::String(isFirstNote ? "yes" : "no"));
            
            // Force an immediate Game of Life update to respond to the note
            if (gameOfLifeEnabled)
            {
                // Update the Game of Life grid
                if (isFirstNote)
                {
                    // For the first note, update the grid and trigger all active cells
                    gameOfLife->update();
                    
                    // Check for active cells and trigger samples immediately after update
                    gameOfLife->checkAndTriggerSamples(
                        drumPads, 
                        ParameterManager::NUM_DRUM_PADS,
                        [this](int col) { return parameterManager->getSampleForColumn(col); },
                        [this](int col) { return parameterManager->getControlModeForColumn(col); },
                        mostRecentMidiNote
                    );
                }
                else
                {
                    // For subsequent notes, only update the pitch of pitched columns
                    // without retriggering all cells
                    gameOfLife->updatePitchOnly(
                        drumPads,
                        ParameterManager::NUM_DRUM_PADS,
                        [this](int col) { return parameterManager->getSampleForColumn(col); },
                        [this](int col) { return parameterManager->getControlModeForColumn(col); },
                        mostRecentMidiNote
                    );
                }
            }
        }
        // Handle MIDI note off messages
        else if (message.isNoteOff())
        {
            int note = message.getNoteNumber();
            
            // Remove note from active notes set
            activeNotes.erase(note);
            
            // Only disable Game of Life if all notes are released
            if (activeNotes.empty())
            {
                gameOfLifeEnabled = false;
                isNoteActive = false;
                
                // Stop all samples when all notes are released
                for (int i = 0; i < ParameterManager::NUM_DRUM_PADS; ++i)
                {
                    drumPads[i].stopSample();
                }
                
                DBG("All notes released, disabling Game of Life and stopping all samples");
            }
            else
            {
                // Update most recent MIDI note to the highest active note
                mostRecentMidiNote = *activeNotes.rbegin();
                
                // Update the pitch of pitched columns without retriggering
                gameOfLife->updatePitchOnly(
                    drumPads,
                    ParameterManager::NUM_DRUM_PADS,
                    [this](int col) { return parameterManager->getSampleForColumn(col); },
                    [this](int col) { return parameterManager->getControlModeForColumn(col); },
                    mostRecentMidiNote
                );
                
                DBG("Note off: " + juce::String(note) + 
                    ", still have " + juce::String(activeNotes.size()) + 
                    " active notes, new most recent: " + juce::String(mostRecentMidiNote));
            }
        }
        // Handle MIDI clock messages
        else if (message.isMidiClock())
        {
            processMidiClock();
        }
        // We still process MIDI start/stop/continue messages, but don't depend on them for simulation
        else if (message.isMidiStart())
        {
            midiClockCounter = 0;
        }
        else if (message.isMidiStop())
        {
            // No action
        }
        else if (message.isMidiContinue())
        {
            // No action
        }
    }
    
    // If we're in standalone mode or the host doesn't send MIDI clock,
    // we need to manually increment the MIDI clock counter based on time
    {
        // Get the current BPM from the host if available, or use default
        double currentBPM = 120.0; // Default BPM
        if (getPlayHead() != nullptr)
        {
            if (auto positionInfo = getPlayHead()->getPosition())
            {
                if (positionInfo->getBpm().hasValue())
                {
                    currentBPM = *positionInfo->getBpm();
                }
            }
        }
        
        // Calculate samples per MIDI clock tick based on BPM
        // At 120 BPM, a quarter note is 0.5 seconds
        // So a MIDI clock tick happens every 0.5/24 = 0.0208 seconds
        // At a sample rate of 44100, that's about 917 samples per MIDI clock tick
        double quarterNoteTimeInSeconds = 60.0 / currentBPM;
        double clockTickTimeInSeconds = quarterNoteTimeInSeconds / 24.0;
        int samplesPerMidiClock = static_cast<int>(clockTickTimeInSeconds * getSampleRate());
        
        static int sampleCounter = 0;
        
        sampleCounter += buffer.getNumSamples();
        
        if (sampleCounter >= samplesPerMidiClock)
        {
            // Increment MIDI clock counter
            midiClockCounter++;
            sampleCounter -= samplesPerMidiClock;
            
            // Debug output
            if (midiClockCounter % 24 == 0) // Log every quarter note
            {
                DBG("MIDI Clock: " + juce::String(midiClockCounter) + 
                    ", Game of Life enabled: " + juce::String(gameOfLifeEnabled ? "yes" : "no") + 
                    ", Note active: " + juce::String(isNoteActive ? "yes" : "no"));
            }
            
            // Process Game of Life update on MIDI clock tick if enabled
            if (gameOfLifeEnabled && isNoteActive)
            {
                // Calculate the interval for Game of Life updates based on interval settings
                int gameOfLifeInterval = calculateIntervalInTicks();
                
                // Update the Game of Life state based on MIDI clock and the selected interval
                if (midiClockCounter % gameOfLifeInterval == 0)
                {
                    // Debug output
                    DBG("Updating Game of Life grid at MIDI clock: " + juce::String(midiClockCounter) +
                        ", interval: " + juce::String(gameOfLifeInterval) +
                        ", active notes: " + juce::String(activeNotes.size()));
                    
                    // Update the Game of Life grid
                    gameOfLife->update();
                    
                    // Check for newly activated cells and trigger samples immediately after update
                    gameOfLife->checkAndTriggerSamples(
                        drumPads, 
                        ParameterManager::NUM_DRUM_PADS,
                        [this](int col) { return parameterManager->getSampleForColumn(col); },
                        [this](int col) { return parameterManager->getControlModeForColumn(col); },
                        mostRecentMidiNote // Pass the most recent MIDI note for pitch control
                    );
                    
                    // Check for deactivated cells and stop samples immediately
                    gameOfLife->checkAndStopSamples(
                        drumPads,
                        ParameterManager::NUM_DRUM_PADS,
                        [this](int col) { return parameterManager->getSampleForColumn(col); }
                    );
                }
            }
        }
    }
    
    // Process audio for each drum pad
    for (int i = 0; i < ParameterManager::NUM_DRUM_PADS; ++i)
    {
        // Process audio for this pad
        drumPads[i].processAudio(buffer, 0, buffer.getNumSamples());
    }
    
    // Copy data to visualization buffer
    visualizationBuffer.clear();
    if (buffer.getNumChannels() > 0 && buffer.getNumSamples() > 0)
    {
        int numSamples = juce::jmin(visualizationBuffer.getNumSamples(), buffer.getNumSamples());
        visualizationBuffer.copyFrom(0, 0, buffer.getReadPointer(0), numSamples);
    }
}

//==============================================================================
bool DrumMachineAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DrumMachineAudioProcessor::createEditor()
{
    return new DrumMachineAudioProcessorEditor (*this);
}

//==============================================================================
void DrumMachineAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Create a root XML element to store all state
    juce::XmlElement rootXml("SynthOfLifeState");
    
    // Store parameters
    auto state = parameterManager->getAPVTS().copyState();
    std::unique_ptr<juce::XmlElement> paramsXml(state.createXml());
    paramsXml->setTagName("Parameters");
    rootXml.addChildElement(paramsXml.release());
    
    // Store sample paths
    juce::XmlElement* samplesXml = rootXml.createNewChildElement("Samples");
    for (int i = 0; i < ParameterManager::NUM_DRUM_PADS; ++i)
    {
        juce::XmlElement* padXml = samplesXml->createNewChildElement("Pad");
        padXml->setAttribute("index", i);
        padXml->setAttribute("path", drumPads[i].getSamplePath());
    }
    
    // Write the XML to the memory block
    copyXmlToBinary(rootXml, destData);
}

void DrumMachineAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Parse the XML from the memory block
    std::unique_ptr<juce::XmlElement> rootXml(getXmlFromBinary(data, sizeInBytes));
    
    if (rootXml != nullptr && rootXml->hasTagName("SynthOfLifeState"))
    {
        // Restore parameters
        juce::XmlElement* paramsXml = rootXml->getChildByName("Parameters");
        if (paramsXml != nullptr && paramsXml->hasTagName(parameterManager->getAPVTS().state.getType()))
        {
            parameterManager->getAPVTS().replaceState(juce::ValueTree::fromXml(*paramsXml));
        }
        
        // Restore sample paths
        juce::XmlElement* samplesXml = rootXml->getChildByName("Samples");
        if (samplesXml != nullptr)
        {
            // Debug output
            DBG("Found samples XML with " + juce::String(samplesXml->getNumChildElements()) + " pad elements");
            
            // Process each pad element
            for (auto* padXml : samplesXml->getChildWithTagNameIterator("Pad"))
            {
                int index = padXml->getIntAttribute("index", -1);
                juce::String path = padXml->getStringAttribute("path", "");
                
                // Debug output
                DBG("Loading sample for pad " + juce::String(index) + " from path: " + path);
                
                // Load the sample if the path is valid
                if (index >= 0 && index < ParameterManager::NUM_DRUM_PADS && path.isNotEmpty())
                {
                    juce::File sampleFile(path);
                    if (sampleFile.existsAsFile())
                    {
                        DBG("File exists, loading sample...");
                        drumPads[index].loadSample(sampleFile);
                        // Store the path for future reference
                        drumPads[index].setSamplePath(path);
                    }
                    else
                    {
                        DBG("File does not exist: " + path);
                    }
                }
            }
        }
        else
        {
            DBG("No samples XML found in state");
        }
    }
    else
    {
        DBG("Invalid or missing root XML element in state");
    }
}

//==============================================================================
// Custom methods

void DrumMachineAudioProcessor::triggerSample(int padIndex, float velocity)
{
    if (padIndex >= 0 && padIndex < ParameterManager::NUM_DRUM_PADS)
    {
        drumPads[padIndex].triggerSample(velocity);
    }
}

void DrumMachineAudioProcessor::triggerSampleWithPitch(int padIndex, float velocity, int pitchShiftSemitones)
{
    if (padIndex >= 0 && padIndex < ParameterManager::NUM_DRUM_PADS)
    {
        drumPads[padIndex].triggerSampleWithPitch(velocity, pitchShiftSemitones);
    }
}

void DrumMachineAudioProcessor::processMidiClock()
{
    // Increment clock counter
    midiClockCounter++;
}

int DrumMachineAudioProcessor::calculateIntervalInTicks()
{
    // MIDI clock sends 24 ticks per quarter note
    
    // Get interval value
    IntervalValue intervalValue = static_cast<IntervalValue>(
        parameterManager->getIntervalValueParam()->getIndex());
        
    // Get interval type
    IntervalType intervalType = static_cast<IntervalType>(
        parameterManager->getIntervalTypeParam()->getIndex());
        
    // Calculate base ticks
    int baseTicks = 0;
    
    switch (intervalValue)
    {
        case IntervalValue::Quarter:
            baseTicks = 24; // 24 ticks per quarter note
            break;
            
        case IntervalValue::Eighth:
            baseTicks = 12; // 12 ticks per eighth note
            break;
            
        case IntervalValue::Sixteenth:
            baseTicks = 6; // 6 ticks per sixteenth note
            break;
            
        default:
            baseTicks = 6; // Default to sixteenth notes
            break;
    }
    
    // Apply interval type modifier
    switch (intervalType)
    {
        case IntervalType::Normal:
            return baseTicks;
            
        case IntervalType::Dotted:
            return baseTicks * 3 / 2; // Dotted note is 1.5x the length
            
        case IntervalType::Triplet:
            return baseTicks * 2 / 3; // Triplet note is 2/3 the length
            
        default:
            return baseTicks;
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DrumMachineAudioProcessor();
}
