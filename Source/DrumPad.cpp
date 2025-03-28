#include "DrumPad.h"

DrumPad::DrumPad()
{
    // Initialize with empty buffer
    sampleBuffer.setSize(2, 0);
    
    // Initialize maximum polyphony
    maxPolyphony = 4;
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
        
        // Store the sample path for state saving/loading
        samplePath = file.getFullPathName();
        
        // Reset playback state
        activeVoices.clear();
    }
}

void DrumPad::triggerSample(float velocity)
{
    // Only trigger if we have a sample loaded
    if (sampleBuffer.getNumSamples() > 0)
    {
        // Check if we've reached the maximum polyphony
        if (activeVoices.size() >= static_cast<size_t>(maxPolyphony))
        {
            // Remove the oldest voice if we've reached the polyphony limit
            activeVoices.erase(activeVoices.begin());
        }
        
        // Create a new voice
        Voice newVoice;
        newVoice.playbackPosition = 0;
        newVoice.volume = velocity; // Use velocity for volume
        newVoice.playbackRate = 1.0f; // Normal playback rate
        
        // Add the new voice
        activeVoices.push_back(newVoice);
        
        // Debug output
        DBG("Triggering sample with velocity: " + juce::String(velocity) + 
            " (Active voices: " + juce::String(activeVoices.size()) + 
            " of " + juce::String(maxPolyphony) + ")");
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
        // Check if we've reached the maximum polyphony
        if (activeVoices.size() >= static_cast<size_t>(maxPolyphony))
        {
            // Remove the oldest voice if we've reached the polyphony limit
            activeVoices.erase(activeVoices.begin());
        }
        
        // Create a new voice
        Voice newVoice;
        newVoice.playbackPosition = 0;
        newVoice.volume = velocity;
        
        // Calculate playback rate based on pitch shift
        // Each semitone is a factor of 2^(1/12)
        newVoice.playbackRate = std::pow(2.0f, pitchShiftSemitones / 12.0f);
        
        // Add the new voice
        activeVoices.push_back(newVoice);
        
        // Debug output
        DBG("Triggering sample with velocity: " + juce::String(velocity) + 
            " and pitch shift: " + juce::String(pitchShiftSemitones) + 
            " (Active voices: " + juce::String(activeVoices.size()) + 
            " of " + juce::String(maxPolyphony) + ")");
    }
    else
    {
        // Debug output if no sample is loaded
        DBG("Cannot trigger sample with pitch - no sample loaded");
    }
}

void DrumPad::stopSample()
{
    if (!activeVoices.empty())
    {
        // Clear all active voices
        activeVoices.clear();
        
        // Debug output
        DBG("Stopping all sample playback");
    }
}

void DrumPad::triggerSampleForCell(float velocity, int cellX, int cellY)
{
    // Only trigger if we have a sample loaded
    if (sampleBuffer.getNumSamples() > 0)
    {
        // Check if we've reached the maximum polyphony
        if (activeVoices.size() >= static_cast<size_t>(maxPolyphony))
        {
            // Remove the oldest voice if we've reached the polyphony limit
            activeVoices.erase(activeVoices.begin());
        }
        
        // Create a new voice
        Voice newVoice;
        newVoice.playbackPosition = 0;
        newVoice.volume = velocity; // Use velocity for volume
        newVoice.playbackRate = 1.0f; // Normal playback rate
        newVoice.cellX = cellX;
        newVoice.cellY = cellY;
        
        // Add the new voice
        activeVoices.push_back(newVoice);
        
        // Debug output
        DBG("Triggering sample for cell (" + juce::String(cellX) + ", " + juce::String(cellY) + 
            ") with velocity: " + juce::String(velocity) + 
            " (Active voices: " + juce::String(activeVoices.size()) + 
            " of " + juce::String(maxPolyphony) + ")");
    }
    else
    {
        // Debug output if no sample is loaded
        DBG("Cannot trigger sample - no sample loaded");
    }
}

void DrumPad::triggerSampleWithPitchForCell(float velocity, int pitchShiftSemitones, int cellX, int cellY)
{
    // Only trigger if we have a sample loaded
    if (sampleBuffer.getNumSamples() > 0)
    {
        // Check if we've reached the maximum polyphony
        if (activeVoices.size() >= static_cast<size_t>(maxPolyphony))
        {
            // Remove the oldest voice if we've reached the polyphony limit
            activeVoices.erase(activeVoices.begin());
        }
        
        // Create a new voice
        Voice newVoice;
        newVoice.playbackPosition = 0;
        newVoice.volume = velocity;
        newVoice.cellX = cellX;
        newVoice.cellY = cellY;
        
        // Calculate playback rate based on pitch shift
        // Each semitone is a factor of 2^(1/12)
        newVoice.playbackRate = std::pow(2.0f, pitchShiftSemitones / 12.0f);
        
        // Add the new voice
        activeVoices.push_back(newVoice);
        
        // Debug output
        DBG("Triggering sample for cell (" + juce::String(cellX) + ", " + juce::String(cellY) + 
            ") with velocity: " + juce::String(velocity) + 
            " and pitch shift: " + juce::String(pitchShiftSemitones) + 
            " (Active voices: " + juce::String(activeVoices.size()) + 
            " of " + juce::String(maxPolyphony) + ")");
    }
    else
    {
        // Debug output if no sample is loaded
        DBG("Cannot trigger sample with pitch - no sample loaded");
    }
}

void DrumPad::stopSampleForCell(int cellX, int cellY)
{
    // Remove voices associated with the specified cell
    auto it = std::remove_if(activeVoices.begin(), activeVoices.end(),
        [cellX, cellY](const Voice& voice) {
            return voice.cellX == cellX && voice.cellY == cellY;
        });
    
    if (it != activeVoices.end())
    {
        // Count how many voices were removed
        size_t removedCount = std::distance(it, activeVoices.end());
        
        // Erase the removed voices
        activeVoices.erase(it, activeVoices.end());
        
        // Debug output
        DBG("Stopped " + juce::String(removedCount) + " voice(s) for cell (" + 
            juce::String(cellX) + ", " + juce::String(cellY) + 
            ") (Remaining voices: " + juce::String(activeVoices.size()) + ")");
    }
}

void DrumPad::processAudio(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    // Skip if muted or no sample loaded
    if (muted || sampleBuffer.getNumSamples() == 0 || activeVoices.empty())
        return;
    
    // Process each active voice
    for (auto it = activeVoices.begin(); it != activeVoices.end(); )
    {
        Voice& voice = *it;
        
        // Calculate pan gains
        float leftGain = voice.volume * volume * (pan <= 0.0f ? 1.0f : 1.0f - pan);
        float rightGain = voice.volume * volume * (pan >= 0.0f ? 1.0f : 1.0f + pan);
        
        // Get number of samples remaining in the source buffer
        int samplesRemaining = sampleBuffer.getNumSamples() - voice.playbackPosition;
        
        // If using pitch shifting, we need to calculate how many source samples to read
        int samplesToProcess;
        
        if (voice.playbackRate != 1.0f)
        {
            // Calculate how many source samples we need based on playback rate
            float sourceSamplesToProcess = numSamples * voice.playbackRate;
            samplesToProcess = juce::jmin(static_cast<int>(sourceSamplesToProcess), samplesRemaining);
            
            // Simple linear interpolation resampling
            for (int channel = 0; channel < juce::jmin(buffer.getNumChannels(), sampleBuffer.getNumChannels()); ++channel)
            {
                float channelGain = channel == 0 ? leftGain : rightGain;
                
                for (int i = 0; i < numSamples; ++i)
                {
                    // Calculate the exact sample position in the source buffer
                    float sourcePos = voice.playbackPosition + i * voice.playbackRate;
                    
                    // Check if we've reached the end of the sample
                    if (sourcePos >= sampleBuffer.getNumSamples())
                    {
                        // We've finished playing this voice
                        it = activeVoices.erase(it);
                        goto nextVoice; // Skip to the next voice
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
            voice.playbackPosition += static_cast<int>(numSamples * voice.playbackRate);
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
                               voice.playbackPosition, 
                               samplesToProcess, 
                               channelGain);
            }
            
            // Update playback position
            voice.playbackPosition += samplesToProcess;
        }
        
        // Check if we've reached the end of the sample
        if (voice.playbackPosition >= sampleBuffer.getNumSamples())
        {
            // Remove this voice
            it = activeVoices.erase(it);
        }
        else
        {
            // Move to the next voice
            ++it;
        }
        
    nextVoice:
        continue;
    }
}
