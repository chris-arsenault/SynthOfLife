#include "DrumPad.h"
#include "DebugLogger.h"

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
    
    // Initialize refresh counter
    refreshCounter = 0;
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

void DrumPad::triggerSampleUnified(float velocity, int pitchShiftSemitones, int cellX, int cellY, float delayMs)
{
    // Note: The delayMs parameter is handled at the processor level through the scheduleSampleWithDelay method.
    // This function is designed to be called directly for immediate playback or indirectly through the scheduler.
    
    if (sampleBuffer.getNumSamples() == 0 || muted)
        return;
    
    // Determine the actual pitch shift to apply based on the pitch control settings
    int actualPitchShift = 0;
    
    // Apply MIDI pitch if enabled (base note from midiNote)
    if (midiPitchEnabled)
    {
        // For both MIDI-triggered and grid-triggered notes, apply the pitch shift
        actualPitchShift = pitchShiftSemitones;
        
        // Add additional debug logging
        DebugLogger::log("DrumPad::triggerSampleUnified - MIDI Pitch Enabled with shift: " + 
                        std::to_string(pitchShiftSemitones) + 
                        ", midiNote: " + std::to_string(midiNote));
    }
    
    // Apply row-based pitch if enabled (from the pitchShiftSemitones parameter)
    if (rowPitchEnabled && cellX >= 0 && cellY >= 0)
    {
        actualPitchShift = pitchShiftSemitones;
        
        // Add additional debug logging
        DebugLogger::log("DrumPad::triggerSampleUnified - Row Pitch Enabled with shift: " + 
                        std::to_string(pitchShiftSemitones));
    }
    
    // Calculate the playback rate for the pitch shift
    float pitchRatio = std::pow(2.0f, actualPitchShift / 12.0f);
    
    // Track the last played note and velocity
    lastPlayedNote = midiNote + actualPitchShift;
    lastPlayedVelocity = velocity;
    
    DebugLogger::log("DrumPad::triggerSampleUnified - MIDI Note: " + std::to_string(lastPlayedNote) + 
                    ", Velocity: " + std::to_string(velocity) + 
                    ", Pitch Shift: " + std::to_string(actualPitchShift) + 
                    ", Original Pitch Shift: " + std::to_string(pitchShiftSemitones) +
                    ", MIDI Pitch Enabled: " + std::to_string(midiPitchEnabled) +
                    ", Row Pitch Enabled: " + std::to_string(rowPitchEnabled) +
                    ", Cell: (" + std::to_string(cellX) + "," + std::to_string(cellY) + ")" +
                    ", Sustain Level: " + std::to_string(envelopeProcessor.getSustainLevel()));
    
    // Check if this is a cell-specific trigger
    bool isCellSpecific = (cellX >= 0 && cellY >= 0);
    
    // Check if we're in legato mode and should update existing voices
    if (legatoMode)
    {
        // For cell-specific triggers, find the voice for that cell
        if (isCellSpecific)
        {
            for (auto& voice : activeVoices)
            {
                if (voice.isForCell(cellX, cellY))
                {
                    // Update the velocity and playback rate of the voice
                    voice.setVolume(velocity);
                    voice.setPlaybackRate(pitchRatio);
                    
                    // If the voice is in release phase, reset it to attack phase
                    // But in legato mode, we want to preserve both envelope and playback position
                    if (voice.isReleasingState())
                    {
                        // Only update the releasing flag but don't reset the envelope
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
        // For non-cell-specific triggers, update all active voices
        else if (!activeVoices.empty())
        {
            for (auto& voice : activeVoices)
            {
                // Update the velocity and playback rate of the voice
                voice.setVolume(velocity);
                voice.setPlaybackRate(pitchRatio);
                
                // If the voice is in release phase, do not reset it to attack phase
                // In legato mode, we want to preserve both envelope and playback position
                if (voice.isReleasingState())
                {
                    // Only update the releasing flag
                    voice.setReleasing(false);
                }
            }
            
            // We've updated all existing voices, so we're done
            return;
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
    
    // Set the cell coordinates if this is a cell-specific trigger
    if (isCellSpecific)
    {
        newVoice.setCell(cellX, cellY);
    }
    
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

void DrumPad::triggerSample(float velocity)
{
    triggerSampleUnified(velocity, 0, -1, -1);
}

void DrumPad::triggerSampleWithPitch(float velocity, int pitchShiftSemitones)
{
    triggerSampleUnified(velocity, pitchShiftSemitones, -1, -1);
}

void DrumPad::triggerSampleForCell(float velocity, int cellX, int cellY)
{
    triggerSampleUnified(velocity, 0, cellX, cellY);
}

void DrumPad::triggerSampleWithPitchForCell(float velocity, int pitchShiftSemitones, int cellX, int cellY)
{
    triggerSampleUnified(velocity, pitchShiftSemitones, cellX, cellY);
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

void DrumPad::updateVoiceParametersForCell(float velocity, int pitchShiftSemitones, int cellX, int cellY)
{
    // Calculate the playback rate for the pitch shift
    float pitchRatio = std::pow(2.0f, pitchShiftSemitones / 12.0f);
    
    // Find the voice for this cell and update its parameters
    for (auto& voice : activeVoices)
    {
        if (voice.isForCell(cellX, cellY))
        {
            // Gently update parameters without resetting envelope or playback position
            voice.setVolume(velocity);
            voice.setPlaybackRate(pitchRatio);
            
            // Log the update
            DebugLogger::log("DrumPad::updateVoiceParametersForCell - Updated voice for cell (" + 
                            std::to_string(cellX) + "," + std::to_string(cellY) + ") - " +
                            "Velocity: " + std::to_string(velocity) + ", " +
                            "Pitch Shift: " + std::to_string(pitchShiftSemitones));
            
            // Found and updated the voice, so we're done
            return;
        }
    }
    
    // If we get here, there was no existing voice for this cell
    // This shouldn't happen in normal operation, but log it just in case
    DebugLogger::log("DrumPad::updateVoiceParametersForCell - No voice found for cell (" + 
                    std::to_string(cellX) + "," + std::to_string(cellY) + ")");
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
    int activeVoiceCount = 0;
    for (auto it = activeVoices.begin(); it != activeVoices.end();)
    {
        if (it->isActive())
        {
            it->processBlock(buffer, startSample, numSamples, volume, pan);
            ++it;
            ++activeVoiceCount;
        }
        else
        {
            // Remove inactive voices
            it = activeVoices.erase(it);
        }
    }
    
    // Periodically refresh sustained voices
    if (++refreshCounter >= 1000) // Every ~1000 audio blocks
    {
        refreshCounter = 0;
        
        // Refresh any voices in sustain state
        for (auto& voice : activeVoices)
        {
            if (voice.getEnvelopeState() == Voice::EnvelopeState::Sustain)
            {
                // Ensure the envelope level is at the correct sustain level
                voice.setEnvelopeLevel(voice.getSustainLevel());
                
                DebugLogger::log("DrumPad: Refreshed sustained voice - Sustain Level: " + 
                                std::to_string(voice.getSustainLevel()));
            }
        }
    }
    
    if (activeVoiceCount > 0 && activeVoiceCount % 10 == 0) {
        DebugLogger::log("DrumPad::renderNextBlock - Processing " + std::to_string(activeVoiceCount) + 
                        " active voices with volume: " + std::to_string(volume));
    }
}

void DrumPad::renderNextBlockToBus(juce::AudioBuffer<float>& buffer, int startSample, int numSamples, int outputBus)
{
    if (muted || activeVoices.empty())
        return;
    
    // Only process if this drum pad is assigned to the specified output bus
    if (this->outputBus != outputBus)
        return;
        
    // Process each active voice
    int activeVoiceCount = 0;
    for (auto it = activeVoices.begin(); it != activeVoices.end();)
    {
        if (it->isActive())
        {
            it->processBlock(buffer, startSample, numSamples, volume, pan);
            ++it;
            ++activeVoiceCount;
        }
        else
        {
            // Remove inactive voices
            it = activeVoices.erase(it);
        }
    }
    
    // Periodically refresh sustained voices
    if (++refreshCounter >= 1000) // Every ~1000 audio blocks
    {
        refreshCounter = 0;
        
        // Refresh any voices in sustain state
        for (auto& voice : activeVoices)
        {
            if (voice.getEnvelopeState() == Voice::EnvelopeState::Sustain)
            {
                // Ensure the envelope level is at the correct sustain level
                voice.setEnvelopeLevel(voice.getSustainLevel());
                
                DebugLogger::log("DrumPad: Refreshed sustained voice - Sustain Level: " + 
                                std::to_string(voice.getSustainLevel()));
            }
        }
    }
    
    if (activeVoiceCount > 0 && activeVoiceCount % 10 == 0) {
        DebugLogger::log("DrumPad::renderNextBlockToBus - Processing " + std::to_string(activeVoiceCount) + 
                        " active voices with volume: " + std::to_string(volume) + 
                        " on output bus: " + std::to_string(outputBus));
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

juce::String DrumPad::getLastPlayedNoteAsString() const
{
    // If no note has been played yet, return a dash
    if (lastPlayedNote <= 0)
        return "-";
    
    // Convert MIDI note number to note name and octave
    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    
    // Determine which note to display
    int displayNote;
    
    // If neither MIDI pitch nor row pitch is enabled, display middle C (MIDI note 60)
    if (!midiPitchEnabled && !rowPitchEnabled)
    {
        displayNote = 60; // Middle C
    }
    else
    {
        // If pitch control is enabled, use the actual last played note
        displayNote = lastPlayedNote;
    }
    
    // MIDI note 60 is middle C (C4 in scientific pitch notation)
    int octave = (displayNote / 12) - 1;
    int noteIndex = displayNote % 12;
    
    return juce::String(noteNames[noteIndex]) + juce::String(octave);
}
