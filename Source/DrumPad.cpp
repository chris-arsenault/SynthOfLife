#include "DrumPad.h"

DrumPad::DrumPad()
{
    // Initialize with empty buffer
    sampleBuffer.setSize(2, 0);
    
    // Initialize maximum polyphony
    maxPolyphony = 4;
    
    // Initialize envelope processor
    envelopeProcessor.setAttackTime(10.0f);
    envelopeProcessor.setDecayTime(100.0f);
    envelopeProcessor.setSustainLevel(0.7f);
    envelopeProcessor.setReleaseTime(200.0f);
    
    // Initialize last played note and velocity
    lastPlayedNote = 0;
    lastPlayedVelocity = 0.0f;
}

DrumPad::~DrumPad()
{
    // Nothing to clean up
}

void DrumPad::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Store the sample rate for pitch shifting calculations
    currentSampleRate = sampleRate;
    
    // Reset playback state
    activeVoices.clear();
    
    // Update envelope processor with the new sample rate
    envelopeProcessor.setSampleRate(sampleRate);
    
    // Set the sample rate for each voice
    for (auto& voice : activeVoices)
    {
        voice.setSampleRate(sampleRate);
    }
}

void DrumPad::loadSample(const juce::File& file)
{
    // Load the audio file
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    
    if (reader != nullptr)
    {
        // Resize the buffer and load the sample
        sampleBuffer.setSize(reader->numChannels, static_cast<int>(reader->lengthInSamples));
        reader->read(&sampleBuffer, 
                    0, 
                    static_cast<int>(reader->lengthInSamples), 
                    0, 
                    true, 
                    true);
                    
        // Store the file path
        filePath = file.getFullPathName();
        
        // Reset playback state
        activeVoices.clear();
    }
}

void DrumPad::triggerSample(float velocity)
{
    if (sampleBuffer.getNumSamples() == 0 || muted)
        return;
    
    // Track the last played note and velocity
    lastPlayedNote = midiNote;
    lastPlayedVelocity = velocity;
    
    // Check if we're in legato mode and there are already active voices
    if (legatoMode && !activeVoices.empty())
    {
        // In legato mode, we update the existing voices instead of creating new ones
        for (auto& voice : activeVoices)
        {
            // Update the velocity (volume) of the voice
            voice.setVolume(velocity);
            
            // If the voice is in release phase, reset it to attack phase
            if (voice.isReleasingState())
            {
                voice.resetEnvelope();
                voice.setReleasing(false);
                
                // Set the envelope parameters
                voice.setEnvelopeRates(
                    envelopeProcessor.getAttackRate(),
                    envelopeProcessor.getDecayRate(),
                    envelopeProcessor.getSustainLevel(),
                    envelopeProcessor.getReleaseRate()
                );
                
                // Debug output
                DBG("Legato mode - Resetting voice envelope with ADSR rates: " +
                    juce::String(envelopeProcessor.getAttackRate()) + ", " +
                    juce::String(envelopeProcessor.getDecayRate()) + ", " +
                    juce::String(envelopeProcessor.getSustainLevel()) + ", " +
                    juce::String(envelopeProcessor.getReleaseRate()));
            }
        }
    }
    else
    {
        // Create a new voice
        Voice newVoice;
        newVoice.setPlaybackPosition(0);
        newVoice.setPlaybackRate(1.0f);
        newVoice.setVolume(velocity);
        
        // Set the sample buffer for the voice
        newVoice.setSampleBuffer(&sampleBuffer);
        
        // Set the sample rate for the voice
        newVoice.setSampleRate(currentSampleRate);
        
        // Get the current ADSR rates from the envelope processor
        float attackRate = envelopeProcessor.getAttackRate();
        float decayRate = envelopeProcessor.getDecayRate();
        float sustainLevel = envelopeProcessor.getSustainLevel();
        float releaseRate = envelopeProcessor.getReleaseRate();
        
        // Debug output
        DBG("New voice - Setting ADSR rates: " +
            juce::String(attackRate) + ", " +
            juce::String(decayRate) + ", " +
            juce::String(sustainLevel) + ", " +
            juce::String(releaseRate) +
            " (from ADSR times: " +
            juce::String(envelopeProcessor.getAttackTime()) + "ms, " +
            juce::String(envelopeProcessor.getDecayTime()) + "ms, " +
            juce::String(envelopeProcessor.getSustainLevel()) + ", " +
            juce::String(envelopeProcessor.getReleaseTime()) + "ms)");
        
        // Set envelope parameters using setEnvelopeRates
        newVoice.setEnvelopeRates(
            attackRate,
            decayRate,
            sustainLevel,
            releaseRate
        );
        
        // Reset the envelope to ensure it starts from the beginning
        newVoice.resetEnvelope();
        
        // Add the voice to the active voices list, limiting polyphony
        if (activeVoices.size() >= maxPolyphony)
        {
            // Remove the oldest voice
            activeVoices.erase(activeVoices.begin());
        }
        
        activeVoices.push_back(newVoice);
    }
}

void DrumPad::triggerSampleWithPitch(float velocity, int pitchShiftSemitones)
{
    if (sampleBuffer.getNumSamples() == 0 || muted)
        return;
    
    // Calculate the playback rate for the pitch shift
    float pitchRatio = std::pow(2.0f, pitchShiftSemitones / 12.0f);
    
    // Track the last played note and velocity
    lastPlayedNote = midiNote + pitchShiftSemitones;
    lastPlayedVelocity = velocity;
    
    // Check if we're in legato mode and there are already active voices
    if (legatoMode && !activeVoices.empty())
    {
        // In legato mode, we update the existing voices instead of creating new ones
        for (auto& voice : activeVoices)
        {
            // Update the velocity (volume) and playback rate of the voice
            voice.setVolume(velocity);
            voice.setPlaybackRate(pitchRatio);
            
            // If the voice is in release phase, reset it to attack phase
            if (voice.isReleasingState())
            {
                voice.resetEnvelope();
                voice.setReleasing(false);
                
                // Set the envelope parameters
                voice.setEnvelopeRates(
                    envelopeProcessor.getAttackRate(),
                    envelopeProcessor.getDecayRate(),
                    envelopeProcessor.getSustainLevel(),
                    envelopeProcessor.getReleaseRate()
                );
            }
        }
    }
    else
    {
        // Create a new voice
        Voice newVoice;
        newVoice.setPlaybackPosition(0);
        newVoice.setPlaybackRate(pitchRatio);
        newVoice.setVolume(velocity);
        
        // Set the sample buffer for the voice
        newVoice.setSampleBuffer(&sampleBuffer);
        
        // Set the sample rate for the voice
        newVoice.setSampleRate(currentSampleRate);
        
        // Set envelope parameters
        newVoice.setEnvelopeRates(
            envelopeProcessor.getAttackRate(),
            envelopeProcessor.getDecayRate(),
            envelopeProcessor.getSustainLevel(),
            envelopeProcessor.getReleaseRate()
        );
        
        // Reset the envelope to ensure it starts from the beginning
        newVoice.resetEnvelope();
        
        // Add the voice to the active voices list, limiting polyphony
        if (activeVoices.size() >= maxPolyphony)
        {
            // Remove the oldest voice
            activeVoices.erase(activeVoices.begin());
        }
        
        activeVoices.push_back(newVoice);
    }
}

void DrumPad::triggerSampleForCell(float velocity, int cellX, int cellY)
{
    if (sampleBuffer.getNumSamples() == 0 || muted)
        return;
    
    // Track the last played note and velocity
    lastPlayedNote = midiNote;
    lastPlayedVelocity = velocity;
    
    // Check if we're in legato mode and there's already a voice for this cell
    if (legatoMode)
    {
        // Find an existing voice for this cell
        for (auto& voice : activeVoices)
        {
            if (voice.isForCell(cellX, cellY))
            {
                // Update the velocity (volume) of the voice
                voice.setVolume(velocity);
                
                // If the voice is in release phase, reset it to attack phase
                if (voice.isReleasingState())
                {
                    voice.resetEnvelope();
                    voice.setReleasing(false);
                    
                    // Set the envelope parameters
                    voice.setEnvelopeRates(
                        envelopeProcessor.getAttackRate(),
                        envelopeProcessor.getDecayRate(),
                        envelopeProcessor.getSustainLevel(),
                        envelopeProcessor.getReleaseRate()
                    );
                }
                
                // Found and updated an existing voice, so we're done
                return;
            }
        }
    }
    
    // Create a new voice
    Voice newVoice;
    newVoice.setPlaybackPosition(0);
    newVoice.setPlaybackRate(1.0f);
    newVoice.setVolume(velocity);
    
    // Set the sample buffer for the voice
    newVoice.setSampleBuffer(&sampleBuffer);
    
    // Set the sample rate for the voice
    newVoice.setSampleRate(currentSampleRate);
    
    // Set the cell coordinates
    newVoice.setCell(cellX, cellY);
    
    // Get the current ADSR rates from the envelope processor
    float attackRate = envelopeProcessor.getAttackRate();
    float decayRate = envelopeProcessor.getDecayRate();
    float sustainLevel = envelopeProcessor.getSustainLevel();
    float releaseRate = envelopeProcessor.getReleaseRate();
    
    // Set envelope parameters using setEnvelopeRates
    newVoice.setEnvelopeRates(
        attackRate,
        decayRate,
        sustainLevel,
        releaseRate
    );
    
    // Reset the envelope to ensure it starts from the beginning
    newVoice.resetEnvelope();
    
    // Check if we need to remove an old voice due to polyphony limit
    if (activeVoices.size() >= maxPolyphony)
    {
        // Remove the oldest voice
        activeVoices.erase(activeVoices.begin());
    }
    
    // Add the new voice to the active voices list
    activeVoices.push_back(newVoice);
}

void DrumPad::triggerSampleWithPitchForCell(float velocity, int pitchShiftSemitones, int cellX, int cellY)
{
    if (sampleBuffer.getNumSamples() == 0 || muted)
        return;
    
    // Calculate the playback rate for the pitch shift
    float pitchRatio = std::pow(2.0f, pitchShiftSemitones / 12.0f);
    
    // Track the last played note and velocity
    lastPlayedNote = midiNote + pitchShiftSemitones;
    lastPlayedVelocity = velocity;
    
    // Check if we're in legato mode and there's already a voice for this cell
    if (legatoMode)
    {
        // Find an existing voice for this cell
        for (auto& voice : activeVoices)
        {
            if (voice.isForCell(cellX, cellY))
            {
                // Update the velocity (volume) and playback rate of the voice
                voice.setVolume(velocity);
                voice.setPlaybackRate(pitchRatio);
                
                // If the voice is in release phase, reset it to attack phase
                if (voice.isReleasingState())
                {
                    voice.resetEnvelope();
                    voice.setReleasing(false);
                    
                    // Set the envelope parameters
                    voice.setEnvelopeRates(
                        envelopeProcessor.getAttackRate(),
                        envelopeProcessor.getDecayRate(),
                        envelopeProcessor.getSustainLevel(),
                        envelopeProcessor.getReleaseRate()
                    );
                }
                
                // Found and updated an existing voice, so we're done
                return;
            }
        }
    }
    
    // Create a new voice
    Voice newVoice;
    newVoice.setPlaybackPosition(0);
    newVoice.setPlaybackRate(pitchRatio);
    newVoice.setVolume(velocity);
    
    // Set the sample buffer for the voice
    newVoice.setSampleBuffer(&sampleBuffer);
    
    // Set the sample rate for the voice
    newVoice.setSampleRate(currentSampleRate);
    
    // Set the cell coordinates
    newVoice.setCell(cellX, cellY);
    
    // Set envelope parameters
    newVoice.setEnvelopeRates(
        envelopeProcessor.getAttackRate(),
        envelopeProcessor.getDecayRate(),
        envelopeProcessor.getSustainLevel(),
        envelopeProcessor.getReleaseRate()
    );
    
    // Reset the envelope to ensure it starts from the beginning
    newVoice.resetEnvelope();
    
    // Check if we need to remove an old voice due to polyphony limit
    if (activeVoices.size() >= maxPolyphony)
    {
        // Remove the oldest voice
        activeVoices.erase(activeVoices.begin());
    }
    
    // Add the new voice to the active voices list
    activeVoices.push_back(newVoice);
}

juce::String DrumPad::getLastPlayedNoteAsString() const
{
    if (lastPlayedNote <= 0)
        return "-";
        
    // Convert MIDI note number to note name and octave
    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    
    // MIDI note 60 is C3 (standard MIDI octave numbering)
    // Formula: octave = (note / 12) - 1
    // For C3 at 60: (60 / 12) - 1 = 5 - 1 = 4, which is wrong
    // Correct formula: octave = (note / 12) - 5
    int octave = (lastPlayedNote / 12) - 5;
    int noteIndex = lastPlayedNote % 12;
    
    return juce::String(noteNames[noteIndex]) + juce::String(octave);
}

void DrumPad::updatePitchForCell(int pitchShiftSemitones, int cellX, int cellY)
{
    // Only update if we have a sample loaded
    if (sampleBuffer.getNumSamples() > 0)
    {
        bool voiceFound = false;
        
        // Find any voices associated with this cell and update their pitch
        for (auto& voice : activeVoices)
        {
            if (voice.isForCell(cellX, cellY))
            {
                // Calculate playback rate based on pitch shift
                float pitchRatio = std::pow(2.0f, pitchShiftSemitones / 12.0f);
                voice.setPlaybackRate(pitchRatio);
                
                voiceFound = true;
            }
        }
        
        if (!voiceFound)
        {
            DBG("No active voice found for cell (" + juce::String(cellX) + ", " + juce::String(cellY) + ") to update pitch");
        }
    }
    else
    {
        DBG("Cannot update pitch - no sample loaded");
    }
}

void DrumPad::stopSample()
{
    // Move all active voices to release phase instead of clearing them
    for (auto& voice : activeVoices)
    {
        if (!voice.isReleasingState())
        {
            voice.setEnvelopeState(Voice::EnvelopeState::Release);
            voice.setReleasing(true);
        }
    }
}

void DrumPad::stopSampleForCell(int cellX, int cellY)
{
    // Find any voices associated with this cell and move them to release phase
    bool voiceFound = false;
    
    for (auto& voice : activeVoices)
    {
        if (voice.isForCell(cellX, cellY) && !voice.isReleasingState())
        {
            voice.setEnvelopeState(Voice::EnvelopeState::Release);
            voice.setReleasing(true);
            voiceFound = true;
        }
    }
    
    if (!voiceFound)
    {
        DBG("No active voice found for cell (" + juce::String(cellX) + ", " + juce::String(cellY) + ") to stop");
    }
}

void DrumPad::processAudio(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (muted || activeVoices.empty())
        return;
        
    // Process each active voice
    for (auto it = activeVoices.begin(); it != activeVoices.end();)
    {
        if (it->isActive())
        {
            it->processBlock(buffer, startSample, numSamples, volume, pan);
            ++it;
        }
        else
        {
            // Remove inactive voices
            it = activeVoices.erase(it);
        }
    }
}

void DrumPad::renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (muted || activeVoices.empty())
        return;
        
    // Process each active voice
    for (auto it = activeVoices.begin(); it != activeVoices.end();)
    {
        if (it->isActive())
        {
            it->processBlock(buffer, startSample, numSamples, volume, pan);
            ++it;
        }
        else
        {
            // Remove inactive voices
            it = activeVoices.erase(it);
        }
    }
}

void DrumPad::releaseSample()
{
    // Start release phase for all active voices
    for (auto& voice : activeVoices)
    {
        voice.noteOff();
    }
}

void DrumPad::setEnvelopeParameters(float attackTimeMs, float decayTimeMs, float sustainLevel, float releaseTimeMs)
{
    // Set all ADSR parameters at once
    envelopeProcessor.setAttackTime(attackTimeMs);
    envelopeProcessor.setDecayTime(decayTimeMs);
    envelopeProcessor.setSustainLevel(sustainLevel);
    envelopeProcessor.setReleaseTime(releaseTimeMs);
    
    // Update all active voices with the new envelope settings
    for (auto& voice : activeVoices)
    {
        voice.updateEnvelopeParameters(attackTimeMs, decayTimeMs, sustainLevel, releaseTimeMs);
    }
}

void DrumPad::releaseResources()
{
    // Stop any active playback
    stopSample();
    
    // Clear active voices
    activeVoices.clear();
}

float DrumPad::getCurrentVolumeLevel() const
{
    // If there are no active voices, return 0
    if (activeVoices.empty())
        return 0.0f;
    
    // Get the most recently added voice (last in the vector)
    const Voice& mostRecentVoice = activeVoices.back();
    
    // Calculate the current volume level based on:
    // 1. The voice's envelope level
    // 2. The voice's volume (velocity)
    // 3. The drum pad's volume setting
    float envelopeLevel = mostRecentVoice.getEnvelopeLevel();
    float voiceVolume = mostRecentVoice.getVolume();
    
    // Combine all factors to get the final volume level
    float volumeLevel = envelopeLevel * voiceVolume * volume;
    
    return volumeLevel;
}

void DrumPad::setAttack(float attackTimeMs)
{
    // Set the attack time in the envelope processor
    envelopeProcessor.setAttackTime(attackTimeMs);
    
    // Debug output
    DBG("DrumPad: Setting attack time to " + juce::String(attackTimeMs) + " ms");
}

void DrumPad::setDecay(float decayTimeMs)
{
    // Set the decay time in the envelope processor
    envelopeProcessor.setDecayTime(decayTimeMs);
    
    // Debug output
    DBG("DrumPad: Setting decay time to " + juce::String(decayTimeMs) + " ms");
}

void DrumPad::setSustain(float sustainLevel)
{
    // Set the sustain level in the envelope processor
    envelopeProcessor.setSustainLevel(sustainLevel);
    
    // Debug output
    DBG("DrumPad: Setting sustain level to " + juce::String(sustainLevel));
}

void DrumPad::setRelease(float releaseTimeMs)
{
    // Set the release time in the envelope processor
    envelopeProcessor.setReleaseTime(releaseTimeMs);
    
    // Debug output
    DBG("DrumPad: Setting release time to " + juce::String(releaseTimeMs) + " ms");
}

float DrumPad::getAttack() const
{
    return envelopeProcessor.getAttackTime();
}

float DrumPad::getDecay() const
{
    return envelopeProcessor.getDecayTime();
}

float DrumPad::getSustain() const
{
    return envelopeProcessor.getSustainLevel();
}

float DrumPad::getRelease() const
{
    return envelopeProcessor.getReleaseTime();
}
