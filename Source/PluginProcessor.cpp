#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterManager.h" // Added missing include directive
#include "DebugLogger.h"

// ColumnControlMode is defined at the global scope in ParameterManager.h
// No using statement needed

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
    // Initialize debug logging
    DebugLogger::initialize();
    DebugLogger::log("DrumMachineAudioProcessor initialized");
    
    // Create parameter manager
    parameterManager = std::make_unique<ParameterManager>(*this);
    
    // Create Game of Life with reference to parameter manager
    gameOfLife = std::make_unique<GameOfLife>(parameterManager.get());
    
    // Initialize Game of Life with random cells
    gameOfLife->initialize(true);
    
    // Initialize visualization buffer
    visualizationBuffer.setSize(1, 1024);
    visualizationBuffer.clear();
    
    // Initialize MIDI note tracking
    activeNotes = std::set<int>();
    mostRecentMidiNote = MIDDLE_C; // Initialize to middle C
    
    midiClockEnabled = false;
    
    lastGameOfLifeUpdateTime = 0.0; // Initialize last update time
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
    for (int i = 0; i < ParameterManager::NUM_SAMPLES; ++i)
    {
        drumPads[i].prepareToPlay(sampleRate, samplesPerBlock);
        
        // Set the polyphony for each drum pad
        auto* polyphonyParam = parameterManager->getPolyphonyParam(i);
        if (polyphonyParam != nullptr)
        {
            drumPads[i].setPolyphony(polyphonyParam->get());
        }
        
        // Debug output for sample paths
        DBG("Pad " + juce::String(i) + " sample path: " + drumPads[i].getFilePath());
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
    for (int i = 0; i < ParameterManager::NUM_SAMPLES; ++i)
    {
        drumPads[i].releaseResources();
    }
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

    // Clear any output channels that don't contain input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    // Get current playhead info
    juce::AudioPlayHead::CurrentPositionInfo posInfo;
    if (auto* playHead = getPlayHead())
        playHead->getCurrentPosition(posInfo);
        
    // Calculate current time in seconds
    double currentTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    
    // Process any scheduled samples that are due to be triggered
    processScheduledSamples(currentTime);
    
    // Process MIDI messages
    processMidiMessages(midiMessages);
    
    // If any notes are active, update the Game of Life based on tempo
    if (isAnyNoteActive())
    {
        // Always use system time to ensure updates happen regardless of host play state
        double bpm = 120.0; // Default BPM
        
        // Try to get host tempo, but don't rely on host play state
        juce::AudioPlayHead* playHead = getPlayHead();
        juce::AudioPlayHead::CurrentPositionInfo posInfo;
        
        if (playHead != nullptr && playHead->getCurrentPosition(posInfo))
        {
            // Use host tempo if available
            bpm = posInfo.bpm;
        }
        
        // Calculate update interval based on tempo and settings
        int intervalInTicks = calculateIntervalInTicks();
        
        // Convert MIDI ticks to seconds
        // MIDI clock sends 24 ticks per quarter note (24 PPQN)
        // But our intervalInTicks is calculated based on 960 PPQN (JUCE standard)
        double secondsPerBeat = 60.0 / bpm;
        double updateIntervalSeconds = secondsPerBeat * (intervalInTicks / 960.0);
        
        // Debug output
        DBG("BPM: " + juce::String(bpm) + 
            ", Interval in ticks: " + juce::String(intervalInTicks) + 
            ", Update interval: " + juce::String(updateIntervalSeconds) + " seconds");
        
        // Check if it's time to update the grid
        if (currentTime - lastGameOfLifeUpdateTime >= updateIntervalSeconds)
        {
            // Update the Game of Life
            gameOfLife->update();
            
            // Update last update time
            lastGameOfLifeUpdateTime = currentTime;
            
            // Process samples based on the updated grid state
            processGameOfLife();
            
            // Debug output
            DBG("Grid updated at time: " + juce::String(currentTime));
        }
    }
    
    // Update parameters for all drum pads
    for (int i = 0; i < ParameterManager::NUM_SAMPLES; ++i)
    {
        // Update volume and pan
        drumPads[i].setVolume(parameterManager->getVolumeForSample(i));
        drumPads[i].setPan(parameterManager->getPanForSample(i));
        
        // Update velocity mode flag
        auto* velocityModeParam = parameterManager->getVelocityModeParam(i);
        if (velocityModeParam != nullptr)
        {
            // No need to do anything with this parameter yet
        }
        
        // Update MIDI pitch and row pitch control flags
        auto* midiPitchParam = parameterManager->getMidiPitchParam(i);
        auto* rowPitchParam = parameterManager->getRowPitchParam(i);
        if (midiPitchParam != nullptr && rowPitchParam != nullptr)
        {
            drumPads[i].setMidiPitchEnabled(midiPitchParam->get());
            drumPads[i].setRowPitchEnabled(rowPitchParam->get());
            
            // Set the MIDI note value for the drum pad
            int midiNote = parameterManager->getMidiNoteForSample(i);
            drumPads[i].setMidiNote(midiNote);
        }
        
        // Update ADSR parameters
        float attack = parameterManager->getAttackForSample(i);
        float decay = parameterManager->getDecayForSample(i);
        float sustain = parameterManager->getSustainForSample(i);
        float release = parameterManager->getReleaseForSample(i);
        
        drumPads[i].setEnvelopeParameters(attack, decay, sustain, release);
    }
    
    // Clear the output buffer
    buffer.clear();
    
    // Render audio for each drum pad
    for (int i = 0; i < ParameterManager::NUM_SAMPLES; ++i)
    {
        // Skip muted samples
        if (parameterManager->getMuteForSample(i))
            continue;
            
        // Get temporary buffer for this drum pad
        juce::AudioBuffer<float> tempBuffer(buffer.getNumChannels(), buffer.getNumSamples());
        tempBuffer.clear();
        
        // Render audio for this drum pad
        drumPads[i].renderNextBlock(tempBuffer, 0, tempBuffer.getNumSamples());
        
        // Add to main buffer
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            buffer.addFrom(channel, 0, tempBuffer, channel, 0, tempBuffer.getNumSamples());
        }
    }
    
    // Copy audio data to visualization buffer
    if (visualizationBuffer.getNumSamples() != buffer.getNumSamples())
    {
        visualizationBuffer.setSize(1, buffer.getNumSamples(), false, true, true);
    }
    
    // Mix down all channels to mono for visualization
    visualizationBuffer.clear();
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        visualizationBuffer.addFrom(0, 0, buffer, channel, 0, buffer.getNumSamples(), 1.0f / buffer.getNumChannels());
    }
    
    // Update the audio visualizer if available
    if (audioVisualizer != nullptr)
    {
        audioVisualizer->pushBuffer(buffer);
    }
}

void DrumMachineAudioProcessor::processMidiMessages(juce::MidiBuffer& midiMessages)
{
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        
        if (message.isNoteOn())
        {
            int noteNumber = message.getNoteNumber();
            
            // Store the most recent MIDI note for pitch control
            mostRecentMidiNote = noteNumber;
            
            // Check if this is the first note being pressed
            bool wasEmpty = activeNotes.empty();
            
            // Add to active notes set
            activeNotes.insert(noteNumber);
            
            // Debug output
            DBG("MIDI Note On: " + juce::String(noteNumber) + 
                " (Pitch shift from middle C: " + juce::String(noteNumber - MIDDLE_C) + ")");
            
            // If this is the first note, initialize the Game of Life grid
            // and set the lastGameOfLifeUpdateTime to current time
            if (wasEmpty)
            {
                // Always use system time to ensure updates happen regardless of host play state
                lastGameOfLifeUpdateTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
                
                // Process samples based on the current grid state
                processGameOfLife();
            }
        }
        else if (message.isNoteOff())
        {
            int noteNumber = message.getNoteNumber();
            
            // Remove from active notes set
            activeNotes.erase(noteNumber);            
            
            // If there are no more active notes, stop all samples
            if (activeNotes.empty())
            {
                // Only stop samples if we're not in timing control mode
                // In timing control mode, let the grid pattern control when samples stop
                bool timingControlEnabled = false;
                
                // Check if any sample has timing control enabled
                for (int i = 0; i < ParameterManager::NUM_SAMPLES; ++i)
                {
                    if (parameterManager->getTimingModeParam(i)->get())
                    {
                        timingControlEnabled = true;
                        break;
                    }
                }
                
                if (!timingControlEnabled)
                {
                    // Stop all samples to follow their envelope's release phase
                    for (auto& drumPad : drumPads)
                    {
                        drumPad.stopSample();
                    }
                    
                    // Also mark all scheduled samples as inactive
                    for (auto& scheduledSample : scheduledSamples)
                    {
                        scheduledSample.active = false;
                        DebugLogger::log("Marking scheduled sample as inactive");
                    }
                    
                    DebugLogger::log("MIDI Note Off: All notes released, marked all scheduled samples as inactive");
                }
                else
                {
                    DebugLogger::log("MIDI Note Off: All notes released, but samples continue in timing control mode");
                }
            }
        }
        else if (message.isMidiClock())
        {
            // Handle MIDI clock messages
            processMidiClock();
        }
    }
}

void DrumMachineAudioProcessor::processGameOfLife()
{
    // Process each cell in the grid
    for (int i = 0; i < ParameterManager::GRID_SIZE; ++i)
    {
        for (int j = 0; j < ParameterManager::GRID_SIZE; ++j)
        {
            // In the grid display, i is the row and j is the column
            // So we need to use the correct mapping for visual representation
            int row = j;    // y in the visual grid
            int column = i; // x in the visual grid
            
            // Get the current state of this cell
            bool currentState = gameOfLife->getCellState(i, j);
            
            // Map column directly to sample index
            int sampleIndex = column % ParameterManager::NUM_SAMPLES;
            
            // Skip if the sample is muted
            if (parameterManager->getMuteForSample(sampleIndex))
                continue;
            
            // Calculate velocity based on row position (higher rows = higher velocity)
            float velocity = 0.5f + (static_cast<float>(row) / static_cast<float>(ParameterManager::GRID_SIZE)) * 0.5f;
            
            // Calculate pitch shift if needed
            int totalPitchShift = 0;
            
            // Get the MIDI pitch and row pitch settings for this sample
            bool midiPitchEnabled = parameterManager->getMidiPitchParam(sampleIndex)->get();
            bool rowPitchEnabled = parameterManager->getRowPitchParam(sampleIndex)->get();
            
            // Calculate MIDI-based pitch shift if enabled
            if (midiPitchEnabled)
            {
                // Calculate pitch shift based on MIDI note
                int basePitchShift = mostRecentMidiNote - MIDDLE_C;
                totalPitchShift += basePitchShift;
                
                // Debug output to verify MIDI pitch is being applied
                DebugLogger::log("MIDI Pitch enabled for sample " + std::to_string(sampleIndex) + 
                                ", MIDI Note: " + std::to_string(mostRecentMidiNote) + 
                                ", Base Pitch Shift: " + std::to_string(basePitchShift));
            }
            
            // Add row-based pitch offset if enabled
            if (rowPitchEnabled)
            {
                // Add row-based pitch offset using the selected scale
                int rowPitchOffset = parameterManager->getPitchOffsetForRow(row);
                totalPitchShift += rowPitchOffset;
                
                // Debug output to verify row pitch is being applied
                DebugLogger::log("Row Pitch enabled for sample " + std::to_string(sampleIndex) + 
                                ", Row: " + std::to_string(row) + 
                                ", Row Pitch Offset: " + std::to_string(rowPitchOffset));
            }
            
            // Get the control mode for this column
            auto controlMode = parameterManager->getControlModeForColumn(column);
            
            // Calculate timing delay if needed
            float delayMs = 0.0f;
            if (controlMode == ColumnControlMode::Timing || controlMode == ColumnControlMode::Both)
            {
                // Get row-based timing delay (0-160ms)
                delayMs = parameterManager->getTimingDelayForRow(row);
            }
            
            // If velocity mode is not active, use a fixed velocity
            if (controlMode != ColumnControlMode::Velocity && controlMode != ColumnControlMode::Both)
            {
                velocity = 0.8f; // Use a fixed velocity when not in velocity mode
            }
            
            // Check if cell just activated (went from off to on)
            if (gameOfLife->cellJustActivated(i, j))
            {
                // Cell just turned on - trigger sample from beginning
                if (delayMs > 0.0f)
                {
                    // If timing delay is active, schedule the sample to be triggered with a delay
                    scheduleSampleWithDelay(sampleIndex, velocity, totalPitchShift, column, row, delayMs);
                }
                else
                {
                    // Trigger immediately with appropriate pitch shift
                    drumPads[sampleIndex].triggerSampleUnified(velocity, totalPitchShift, column, row);
                }
            }
            else if (currentState && gameOfLife->wasCellActive(i, j))
            {
                // Cell remains on
                bool isLegato = parameterManager->getLegatoForSample(sampleIndex);
                
                if (!isLegato)
                {
                    // Not legato - retrigger sample from beginning
                    if (delayMs > 0.0f)
                    {
                        // If timing delay is active, schedule the sample to be triggered with a delay
                        scheduleSampleWithDelay(sampleIndex, velocity, totalPitchShift, column, row, delayMs);
                    }
                    else
                    {
                        // Trigger immediately with appropriate pitch shift
                        drumPads[sampleIndex].triggerSampleUnified(velocity, totalPitchShift, column, row);
                    }
                }
                // If legato, continue playing the sample (do nothing)
            }
            // Check if cell just deactivated (went from on to off)
            else if (gameOfLife->cellJustDeactivated(i, j))
            {
                // Cell just turned off - stop sample with release for this specific cell
                drumPads[sampleIndex].stopSampleForCell(column, row);
                
                // Also mark any scheduled samples for this cell as inactive
                for (auto& scheduledSample : scheduledSamples)
                {
                    if (scheduledSample.sampleIndex == sampleIndex && 
                        scheduledSample.cellX == column && 
                        scheduledSample.cellY == row)
                    {
                        scheduledSample.active = false;
                        DebugLogger::log("Marking scheduled sample as inactive for cell (" + 
                                        std::to_string(column) + "," + std::to_string(row) + ")");
                    }
                }
            }
        }
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
    juce::XmlElement rootXml("DrumMachineState");
    
    // Store parameters
    auto state = parameterManager->getAPVTS().copyState();
    std::unique_ptr<juce::XmlElement> paramsXml(state.createXml());
    paramsXml->setTagName("Parameters");
    rootXml.addChildElement(paramsXml.release());
    
    // Store sample paths
    juce::XmlElement* samplesXml = rootXml.createNewChildElement("Samples");
    for (int i = 0; i < ParameterManager::NUM_SAMPLES; ++i)
    {
        juce::XmlElement* padXml = samplesXml->createNewChildElement("Pad");
        padXml->setAttribute("index", i);
        padXml->setAttribute("path", drumPads[i].getFilePath());
        
        // Store ADSR envelope parameters
        padXml->setAttribute("attack", drumPads[i].getAttack());
        padXml->setAttribute("decay", drumPads[i].getDecay());
        padXml->setAttribute("sustain", drumPads[i].getSustain());
        padXml->setAttribute("release", drumPads[i].getRelease());
    }
    
    // Write the XML to the memory block
    copyXmlToBinary(rootXml, destData);
}

void DrumMachineAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Parse the XML from the memory block
    std::unique_ptr<juce::XmlElement> rootXml(getXmlFromBinary(data, sizeInBytes));
    
    if (rootXml != nullptr && rootXml->hasTagName("DrumMachineState"))
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
                
                // Load the sample file if it exists
                if (index >= 0 && index < ParameterManager::NUM_SAMPLES && path.isNotEmpty())
                {
                    juce::File sampleFile(path);
                    if (sampleFile.existsAsFile())
                    {
                        drumPads[index].loadSample(sampleFile);
                    }
                    else
                    {
                        DBG("File does not exist: " + path);
                    }
                }
                
                // Restore ADSR envelope parameters
                if (index >= 0 && index < ParameterManager::NUM_SAMPLES)
                {
                    float attack = padXml->getDoubleAttribute("attack", 10.0);
                    float decay = padXml->getDoubleAttribute("decay", 100.0);
                    float sustain = padXml->getDoubleAttribute("sustain", 0.7);
                    float release = padXml->getDoubleAttribute("release", 200.0);
                    
                    // Debug output for ADSR values
                    DBG("Loading ADSR for pad " + juce::String(index) + 
                        ": A=" + juce::String(attack) + 
                        ", D=" + juce::String(decay) + 
                        ", S=" + juce::String(sustain) + 
                        ", R=" + juce::String(release));
                    
                    // Set the ADSR parameters
                    drumPads[index].setAttack(attack);
                    drumPads[index].setDecay(decay);
                    drumPads[index].setSustain(sustain);
                    drumPads[index].setRelease(release);
                }
            }
            
            // Notify listeners that state has been loaded
            for (auto* listener : stateLoadedListeners)
            {
                if (listener != nullptr)
                {
                    listener->stateLoaded();
                }
            }
        }
    }
}

//==============================================================================
// Custom methods

void DrumMachineAudioProcessor::triggerSample(int padIndex, float velocity)
{
    if (padIndex >= 0 && padIndex < drumPads.size())
    {
        drumPads[padIndex].triggerSampleUnified(velocity);
    }
}

void DrumMachineAudioProcessor::triggerSampleWithPitch(int padIndex, float velocity, int pitchShiftSemitones)
{
    if (padIndex >= 0 && padIndex < drumPads.size())
    {
        drumPads[padIndex].triggerSampleUnified(velocity, pitchShiftSemitones);
    }
}

void DrumMachineAudioProcessor::processMidiClock()
{
    // Increment clock counter
    midiClockCounter++;
}

int DrumMachineAudioProcessor::calculateIntervalInTicks()
{
    // JUCE uses 960 PPQN (Pulses Per Quarter Note) standard
    
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
            baseTicks = 960; // 960 ticks per quarter note
            break;
            
        case IntervalValue::Eighth:
            baseTicks = 480; // 480 ticks per eighth note
            break;
            
        case IntervalValue::Sixteenth:
            baseTicks = 240; // 240 ticks per sixteenth note
            break;
            
        default:
            baseTicks = 240; // Default to sixteenth notes
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

void DrumMachineAudioProcessor::notifyStateLoaded()
{
    // Notify all listeners that the state has been loaded
    for (auto* listener : stateLoadedListeners)
    {
        if (listener != nullptr)
            listener->stateLoaded();
    }
}

void DrumMachineAudioProcessor::addStateLoadedListener(StateLoadedListener* listener)
{
    if (listener != nullptr && std::find(stateLoadedListeners.begin(), stateLoadedListeners.end(), listener) == stateLoadedListeners.end())
    {
        stateLoadedListeners.push_back(listener);
    }
}

void DrumMachineAudioProcessor::removeStateLoadedListener(StateLoadedListener* listener)
{
    auto it = std::find(stateLoadedListeners.begin(), stateLoadedListeners.end(), listener);
    if (it != stateLoadedListeners.end())
    {
        stateLoadedListeners.erase(it);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DrumMachineAudioProcessor();
}

void DrumMachineAudioProcessor::scheduleSampleWithDelay(int sampleIndex, float velocity, int pitchShift, 
                                                      int cellX, int cellY, float delayMs)
{
    // Calculate the trigger time based on current time plus delay
    double currentTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    double triggerTime = currentTime + (delayMs / 1000.0); // Convert ms to seconds
    
    // Create a scheduled sample and add it to the queue
    scheduledSamples.emplace_back(sampleIndex, velocity, pitchShift, cellX, cellY, triggerTime);
}

void DrumMachineAudioProcessor::processScheduledSamples(double currentTime)
{
    // Process any scheduled samples that are due to be triggered
    auto it = scheduledSamples.begin();
    while (it != scheduledSamples.end())
    {
        if (currentTime >= it->triggerTime)
        {
            // Only trigger if the sample is still active (cell hasn't been turned off)
            if (it->active)
            {
                // Time to trigger this sample using the unified function
                drumPads[it->sampleIndex].triggerSampleUnified(it->velocity, it->pitchShift, it->cellX, it->cellY);
                DebugLogger::log("Triggering scheduled sample for cell (" + 
                                std::to_string(it->cellX) + "," + std::to_string(it->cellY) + ")");
            }
            else
            {
                DebugLogger::log("Skipping inactive scheduled sample for cell (" + 
                                std::to_string(it->cellX) + "," + std::to_string(it->cellY) + ")");
            }
            
            // Remove this sample from the queue
            it = scheduledSamples.erase(it);
        }
        else
        {
            // Not yet time to trigger this sample
            ++it;
        }
    }
}

//==============================================================================
