#include "Voice.h"
#include "DebugLogger.h"

Voice::Voice()
{
    // Initialize with default values
    playbackPosition = 0;
    volume = 1.0f;
    playbackRate = 1.0f;
    cellX = -1;
    cellY = -1;
    
    // Initialize envelope
    envelopeState = EnvelopeState::Attack;
    envelopeLevel = 0.0f;
    isReleasing = false;
    
    // Default sample rate (will be updated in prepareToPlay)
    currentSampleRate = 44100.0f;
}

void Voice::setEnvelopeRates(float attack, float decay, float sustain, float release)
{
    // Store the ADSR parameters for this voice
    voiceAttackRate = attack;
    voiceDecayRate = decay;
    voiceSustainLevel = sustain;
    voiceReleaseRate = release;
    
    // Debug output to verify the rates
    DBG("Voice ADSR rates set - Attack: " + juce::String(attack) + 
        ", Decay: " + juce::String(decay) + 
        ", Sustain: " + juce::String(sustain) + 
        ", Release: " + juce::String(release));
}

void Voice::setSampleRate(float sampleRate)
{
    currentSampleRate = sampleRate;
}

float Voice::getSampleRate() const
{
    return currentSampleRate;
}

void Voice::updateEnvelope(int numSamples)
{
    // Skip if we're in idle state
    if (envelopeState == EnvelopeState::Idle)
        return;
    
    // Process the envelope based on current state
    switch (envelopeState)
    {
        case EnvelopeState::Attack:
            // Increment envelope level during attack phase
            // Apply the rate for each sample in the block
            if (voiceAttackRate > 0.0f)
            {
                envelopeLevel += voiceAttackRate * numSamples;
                
                // Check if we've reached the target level
                if (envelopeLevel >= 1.0f)
                {
                    envelopeLevel = 1.0f;
                    envelopeState = EnvelopeState::Decay;
                    DebugLogger::log("Voice: Transitioned from Attack to Decay state");
                }
            }
            else
            {
                // If attack rate is 0, jump straight to full level and decay phase
                envelopeLevel = 1.0f;
                envelopeState = EnvelopeState::Decay;
                DebugLogger::log("Voice: Jumped from Attack to Decay state (zero attack rate)");
            }
            break;
            
        case EnvelopeState::Decay:
            // Decrement envelope level during decay phase
            // Apply the rate for each sample in the block
            if (voiceDecayRate > 0.0f)
            {
                envelopeLevel -= voiceDecayRate * numSamples;
                
                // Check if we've reached the sustain level
                if (envelopeLevel <= voiceSustainLevel)
                {
                    envelopeLevel = voiceSustainLevel;
                    envelopeState = EnvelopeState::Sustain;
                    DebugLogger::log("Voice: Transitioned to Sustain state with level: " + std::to_string(voiceSustainLevel));
                }
            }
            else
            {
                // If decay rate is 0, jump straight to sustain level and sustain phase
                envelopeLevel = voiceSustainLevel;
                envelopeState = EnvelopeState::Sustain;
                DebugLogger::log("Voice: Jumped to Sustain state with level: " + std::to_string(voiceSustainLevel));
            }
            break;
            
        case EnvelopeState::Sustain:
            // Always explicitly set the envelope level to the sustain level
            // This ensures it never decreases over time
            envelopeLevel = voiceSustainLevel;
            break;
            
        case EnvelopeState::Release:
            // Decrement envelope level during release phase
            // Apply the rate for each sample in the block
            if (voiceReleaseRate > 0.0f)
            {
                envelopeLevel -= voiceReleaseRate * numSamples;
                
                // Check if we've reached zero
                if (envelopeLevel <= 0.0f)
                {
                    envelopeLevel = 0.0f;
                    envelopeState = EnvelopeState::Idle;
                    DebugLogger::log("Voice: Transitioned to Idle state");
                }
            }
            else
            {
                // If release rate is 0, jump straight to zero level and idle state
                envelopeLevel = 0.0f;
                envelopeState = EnvelopeState::Idle;
                DebugLogger::log("Voice: Jumped to Idle state");
            }
            break;
            
        default:
            break;
    }
}

void Voice::processBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples, float masterVolume, float pan)
{
    // Update the envelope for this voice
    updateEnvelope(numSamples);
    
    // Skip if the voice is no longer active
    if (envelopeState == EnvelopeState::Idle || !sampleBuffer || sampleBuffer->getNumSamples() == 0)
        return;
    
    // Ensure envelope level is correct for sustain state
    if (envelopeState == EnvelopeState::Sustain && envelopeLevel != voiceSustainLevel)
    {
        envelopeLevel = voiceSustainLevel;
        DebugLogger::log("Voice: Fixed envelope level in processBlock: " + std::to_string(voiceSustainLevel));
    }
    
    // Calculate pan gains
    float leftGain = volume * masterVolume * (pan <= 0.0f ? 1.0f : 1.0f - pan);
    float rightGain = volume * masterVolume * (pan >= 0.0f ? 1.0f : 1.0f + pan);
    
    // Apply envelope to gains
    leftGain *= envelopeLevel;
    rightGain *= envelopeLevel;
    
    // Periodically log the envelope level and gain to help diagnose volume issues
    static int logCounter = 0;
    if (++logCounter >= 1000) // Log every 1000 blocks to avoid flooding
    {
        logCounter = 0;
        if (envelopeState == EnvelopeState::Sustain)
        {
            DebugLogger::log("Voice Sustain Stats - Envelope Level: " + std::to_string(envelopeLevel) + 
                            ", Sustain Level: " + std::to_string(voiceSustainLevel) + 
                            ", Volume: " + std::to_string(volume) + 
                            ", Master Volume: " + std::to_string(masterVolume) + 
                            ", Left Gain: " + std::to_string(leftGain) + 
                            ", Right Gain: " + std::to_string(rightGain));
            
            // Ensure envelope level is at sustain level during sustained playback
            if (envelopeLevel != voiceSustainLevel)
            {
                envelopeLevel = voiceSustainLevel;
                DebugLogger::log("Voice: Corrected envelope level to match sustain level: " + std::to_string(voiceSustainLevel));
            }
        }
    }
    
    // Process audio for each channel
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        float channelGain = (channel == 0) ? leftGain : rightGain;
        
        // Add to the buffer
        for (int i = 0; i < numSamples; ++i)
        {
            // Calculate the exact sample position using the playback rate
            float exactSamplePos = playbackPosition + (i * playbackRate);
            int samplePos = static_cast<int>(exactSamplePos);
            
            // Check if we're still within the sample buffer
            if (samplePos < sampleBuffer->getNumSamples())
            {
                // Get the sample from the buffer (use the appropriate channel or mix down)
                float sample;
                if (channel < sampleBuffer->getNumChannels())
                {
                    sample = sampleBuffer->getSample(channel, samplePos);
                }
                else
                {
                    // If we have fewer channels in the sample, use the first channel
                    sample = sampleBuffer->getSample(0, samplePos);
                }
                
                // Apply gain and add to output buffer
                buffer.addSample(channel, startSample + i, sample * channelGain);
            }
        }
    }
    
    // Update playback position
    playbackPosition += static_cast<int>(numSamples * playbackRate);
    
    // Check if we've reached the end of the sample
    if (playbackPosition >= sampleBuffer->getNumSamples())
    {
        // For one-shot samples, transition to release phase
        if (!isReleasing)
        {
            noteOff();
        }
    }
}

void Voice::noteOff()
{
    // Only transition to release state if not already in release or idle
    if (envelopeState != EnvelopeState::Release && envelopeState != EnvelopeState::Idle)
    {
        DebugLogger::log("Voice::noteOff - Transitioning from " + 
                         std::string(envelopeState == EnvelopeState::Attack ? "Attack" : 
                                    envelopeState == EnvelopeState::Decay ? "Decay" : "Sustain") + 
                         " to Release state. Current level: " + std::to_string(envelopeLevel));
        
        envelopeState = EnvelopeState::Release;
        isReleasing = true;
    }
    else
    {
        DebugLogger::log("Voice::noteOff - Already in " + 
                         std::string(envelopeState == EnvelopeState::Release ? "Release" : "Idle") + 
                         " state. Current level: " + std::to_string(envelopeLevel));
    }
}

void Voice::updateEnvelopeParameters(float attackTimeMs, float decayTimeMs, float sustainLevel, float releaseTimeMs)
{
    // Convert times in milliseconds to rates per sample
    float attackRate = 0.0f;
    if (attackTimeMs > 0.0f)
    {
        // Convert attack time from ms to seconds, then to rate per sample
        attackRate = 1.0f / (attackTimeMs * 0.001f * getSampleRate());
    }
    else
    {
        // If attack time is 0, use a very fast attack
        attackRate = 1.0f;
    }
    
    float decayRate = 0.0f;
    if (decayTimeMs > 0.0f)
    {
        // Convert decay time from ms to seconds, then to rate per sample
        decayRate = (1.0f - sustainLevel) / (decayTimeMs * 0.001f * getSampleRate());
    }
    else
    {
        // If decay time is 0, use a very fast decay
        decayRate = 1.0f;
    }
    
    float releaseRate = 0.0f;
    if (releaseTimeMs > 0.0f)
    {
        // Convert release time from ms to seconds, then to rate per sample
        releaseRate = sustainLevel / (releaseTimeMs * 0.001f * getSampleRate());
    }
    else
    {
        // If release time is 0, use a very fast release
        releaseRate = 1.0f;
    }
    
    // Update the envelope parameters
    setEnvelopeRates(attackRate, decayRate, sustainLevel, releaseRate);
}
