#include "DrumPad.h"

DrumPad::DrumPad()
{
    // Initialize with empty buffer
    sampleBuffer.setSize(2, 0);
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
    playing = false;
    playbackPosition = 0;
    playbackRate = 1.0f;
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
        playing = false;
        playbackPosition = 0;
        playbackRate = 1.0f;
    }
}

void DrumPad::triggerSample(float velocity)
{
    // Only trigger if we have a sample loaded
    if (sampleBuffer.getNumSamples() > 0)
    {
        playing = true;
        playbackPosition = 0;
        volume = velocity; // Use velocity for volume
        playbackRate = 1.0f; // Reset to normal playback rate
        
        // Debug output
        DBG("Triggering sample with velocity: " + juce::String(velocity));
    }
    else
    {
        // Debug output if no sample is loaded
        DBG("Cannot trigger sample - no sample loaded");
    }
}

void DrumPad::triggerSampleWithPitch(float velocity, int pitchShiftSemitones)
{
    // Only trigger if we have a sample loaded
    if (sampleBuffer.getNumSamples() > 0)
    {
        playing = true;
        playbackPosition = 0;
        volume = velocity;
        
        // Calculate playback rate based on pitch shift
        // Each semitone is a factor of 2^(1/12)
        playbackRate = std::pow(2.0f, pitchShiftSemitones / 12.0f);
        
        // Debug output
        DBG("Triggering sample with velocity: " + juce::String(velocity) + 
            " and pitch shift: " + juce::String(pitchShiftSemitones));
    }
    else
    {
        // Debug output if no sample is loaded
        DBG("Cannot trigger sample with pitch - no sample loaded");
    }
}

void DrumPad::processAudio(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    // Skip if not playing or muted
    if (!playing || muted || sampleBuffer.getNumSamples() == 0)
        return;
    
    // Calculate pan gains
    float leftGain = volume * (pan <= 0.0f ? 1.0f : 1.0f - pan);
    float rightGain = volume * (pan >= 0.0f ? 1.0f : 1.0f + pan);
    
    // Get number of samples remaining in the source buffer
    int samplesRemaining = sampleBuffer.getNumSamples() - playbackPosition;
    
    // If using pitch shifting, we need to calculate how many source samples to read
    int samplesToProcess;
    
    if (playbackRate != 1.0f)
    {
        // Calculate how many source samples we need based on playback rate
        float sourceSamplesToProcess = numSamples * playbackRate;
        samplesToProcess = juce::jmin(static_cast<int>(sourceSamplesToProcess), samplesRemaining);
        
        // Simple linear interpolation resampling
        for (int channel = 0; channel < juce::jmin(buffer.getNumChannels(), sampleBuffer.getNumChannels()); ++channel)
        {
            float channelGain = channel == 0 ? leftGain : rightGain;
            
            for (int i = 0; i < numSamples; ++i)
            {
                // Calculate the exact sample position in the source buffer
                float sourcePos = playbackPosition + i * playbackRate;
                
                // Check if we've reached the end of the sample
                if (sourcePos >= sampleBuffer.getNumSamples())
                {
                    // We've finished playing the sample
                    playing = false;
                    playbackPosition = 0;
                    playbackRate = 1.0f; // Reset playback rate
                    break;
                }
                
                // Get the integer part of the position
                int pos1 = static_cast<int>(sourcePos);
                int pos2 = pos1 + 1;
                
                // Make sure pos2 is within the buffer
                if (pos2 >= sampleBuffer.getNumSamples())
                    pos2 = pos1;
                    
                // Calculate the fractional part for interpolation
                float frac = sourcePos - pos1;
                
                // Get the two sample values
                float sample1 = sampleBuffer.getSample(channel, pos1);
                float sample2 = sampleBuffer.getSample(channel, pos2);
                
                // Linear interpolation
                float interpolatedSample = sample1 + frac * (sample2 - sample1);
                
                // Add to output buffer
                buffer.addSample(channel, startSample + i, interpolatedSample * channelGain);
            }
        }
        
        // Update playback position based on how many source samples we processed
        playbackPosition += static_cast<int>(numSamples * playbackRate);
    }
    else
    {
        // Normal playback (no pitch shifting)
        samplesToProcess = juce::jmin(numSamples, samplesRemaining);
        
        // Mix the sample into the output buffer with panning
        for (int channel = 0; channel < juce::jmin(buffer.getNumChannels(), sampleBuffer.getNumChannels()); ++channel)
        {
            float channelGain = channel == 0 ? leftGain : rightGain;
            
            buffer.addFrom(channel, startSample, 
                           sampleBuffer, 
                           channel, 
                           playbackPosition, 
                           samplesToProcess, 
                           channelGain);
        }
        
        // Update playback position
        playbackPosition += samplesToProcess;
    }
    
    // Check if we've reached the end of the sample
    if (playbackPosition >= sampleBuffer.getNumSamples())
    {
        playing = false;
        playbackPosition = 0;
        playbackRate = 1.0f; // Reset playback rate
    }
}
