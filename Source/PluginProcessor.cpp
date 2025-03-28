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
    
    // Create Game of Life
    gameOfLife = std::make_unique<GameOfLife>();
    
    // Initialize Game of Life with random cells
    gameOfLife->initialize(true);
    
    // Initialize visualization buffer
    visualizationBuffer.setSize(1, 1024);
    visualizationBuffer.clear();
    
    isNoteActive = false;
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
    // Initialize drum pads
    for (auto& pad : drumPads)
    {
        pad.prepareToPlay(sampleRate, samplesPerBlock);
    }
    
    // Reset MIDI clock counter
    midiClockCounter = 0;
    
    // Reset host playback state
    isHostPlaying = false;
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
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Clear the buffer
    buffer.clear();
    
    // Check if we have MIDI messages
    bool hasMidiMessages = !midiMessages.isEmpty();
    
    // Check if host is playing
    if (hasMidiMessages && getPlayHead() != nullptr)
    {
        juce::AudioPlayHead::CurrentPositionInfo posInfo;
        getPlayHead()->getCurrentPosition(posInfo);
        isHostPlaying = posInfo.isPlaying;
    }

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
            
            // Enable Game of Life when any note is pressed
            gameOfLifeEnabled = true;
            isNoteActive = true;
            
            // Check each pad for matching MIDI note
            for (int i = 0; i < ParameterManager::NUM_DRUM_PADS; ++i)
            {
                int padNote = parameterManager->getMidiNoteParam(i)->get();
                if (note == padNote)
                {
                    triggerSample(i, velocity);
                    break;
                }
            }
        }
        // Handle MIDI note off messages
        else if (message.isNoteOff())
        {
            int note = message.getNoteNumber();
            
            // Check if this is the last note being released
            // For simplicity, we'll just disable the Game of Life on any note off
            // In a more complex implementation, we would track all active notes
            gameOfLifeEnabled = false;
            isNoteActive = false;
        }
        // Handle MIDI clock messages
        else if (message.isMidiClock())
        {
            processMidiClock();
        }
        // Handle MIDI start message
        else if (message.isMidiStart())
        {
            isHostPlaying = true;
            midiClockCounter = 0;
        }
        // Handle MIDI stop/continue messages
        else if (message.isMidiStop())
        {
            isHostPlaying = false;
        }
        else if (message.isMidiContinue())
        {
            isHostPlaying = true;
        }
    }
    
    // If we're in standalone mode or the host doesn't send MIDI clock,
    // we need to manually increment the MIDI clock counter
    if (isHostPlaying && !hasMidiMessages)
    {
        // Get the current BPM from the host if available
        double currentBPM = 120.0; // Default BPM
        if (getPlayHead() != nullptr)
        {
            juce::AudioPlayHead::CurrentPositionInfo posInfo;
            getPlayHead()->getCurrentPosition(posInfo);
            currentBPM = posInfo.bpm;
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
        }
    }
    
    // Process Game of Life if enabled and host is playing
    if (gameOfLifeEnabled && isHostPlaying && isNoteActive)
    {
        // Calculate the interval for Game of Life updates based on interval settings
        int gameOfLifeInterval = calculateIntervalInTicks();
        
        // Update the Game of Life state based on MIDI clock and the selected interval
        if (midiClockCounter % gameOfLifeInterval == 0 && midiClockCounter > 0)
        {
            // Update the Game of Life grid
            gameOfLife->update();
            
            // Check for newly activated cells and trigger samples immediately after update
            gameOfLife->checkAndTriggerSamples(
                drumPads, 
                ParameterManager::NUM_DRUM_PADS,
                [this](int col) { return parameterManager->getSampleForColumn(col); },
                [this](int col) { return parameterManager->getControlModeForColumn(col); }
            );
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
    // Store parameters
    auto state = parameterManager->getAPVTS().copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DrumMachineAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore parameters
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameterManager->getAPVTS().state.getType()))
            parameterManager->getAPVTS().replaceState(juce::ValueTree::fromXml(*xmlState));
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
