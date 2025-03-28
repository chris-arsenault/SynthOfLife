#include "Voice.h"

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
                }
            }
            else
            {
                // If attack rate is 0, jump straight to full level and decay phase
                envelopeLevel = 1.0f;
                envelopeState = EnvelopeState::Decay;
            }
            break;
            
        case EnvelopeState::Decay:
            // Decrement envelope level during decay phase
            // Apply the rate for each sample in the block
            envelopeLevel -= voiceDecayRate * numSamples;
            
            // Check if we've reached the sustain level
            if (envelopeLevel <= voiceSustainLevel)
            {
                envelopeLevel = voiceSustainLevel;
                envelopeState = EnvelopeState::Sustain;
            }
            break;
            
        case EnvelopeState::Sustain:
            // Maintain the sustain level
            envelopeLevel = voiceSustainLevel;
            break;
            
        case EnvelopeState::Release:
            // Decrement envelope level during release phase
            // Apply the rate for each sample in the block
            envelopeLevel -= voiceReleaseRate * numSamples;
            
            // Check if we've reached zero
            if (envelopeLevel <= 0.0f)
            {
                envelopeLevel = 0.0f;
                envelopeState = EnvelopeState::Idle;
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
    
    // Calculate pan gains
    float leftGain = volume * masterVolume * (pan <= 0.0f ? 1.0f : 1.0f - pan);
    float rightGain = volume * masterVolume * (pan >= 0.0f ? 1.0f : 1.0f + pan);
    
    // Apply envelope to gains
    leftGain *= envelopeLevel;
    rightGain *= envelopeLevel;
    
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
        envelopeState = EnvelopeState::Release;
        isReleasing = true;
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
