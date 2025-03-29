#pragma once

#include <JuceHeader.h>
#include <memory>

/**
 * Represents a single voice for sample playback with ADSR envelope
 */
class Voice
{
public:
    // Voice state enum for ADSR envelope
    enum class EnvelopeState
    {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    Voice();
    ~Voice() = default;
    
    // Playback position in samples
    int getPlaybackPosition() const { return playbackPosition; }
    void setPlaybackPosition(int position) { playbackPosition = position; }
    void advancePlaybackPosition(int samples) { playbackPosition += samples; }
    
    // Volume (velocity) control
    float getVolume() const { return volume; }
    void setVolume(float newVolume) { volume = newVolume; }
    
    // Playback rate (pitch) control
    float getPlaybackRate() const { return playbackRate; }
    void setPlaybackRate(float rate) { playbackRate = rate; }
    
    // Sample buffer handling
    void setSampleBuffer(const juce::AudioBuffer<float>* buffer) 
    { 
        // Store a reference to the sample buffer
        // We don't need to make a copy since the buffer is owned by the DrumPad
        // and all voices share the same buffer but have independent playback positions
        sampleBuffer = buffer;
    }
    const juce::AudioBuffer<float>* getSampleBuffer() const { return sampleBuffer; }
    
    // Cell coordinates (for Game of Life grid)
    int getCellX() const { return cellX; }
    int getCellY() const { return cellY; }
    void setCell(int x, int y) { cellX = x; cellY = y; }
    bool isForCell(int x, int y) const { return cellX == x && cellY == y; }
    
    // ADSR envelope state
    EnvelopeState getEnvelopeState() const { return envelopeState; }
    void setEnvelopeState(EnvelopeState state) { envelopeState = state; }
    
    float getEnvelopeLevel() const { return envelopeLevel; }
    void setEnvelopeLevel(float level) { envelopeLevel = level; }
    
    bool isReleasingState() const { return isReleasing; }
    void setReleasing(bool releasing) { isReleasing = releasing; }
    
    // Reset envelope to initial attack state and optionally reset playback position
    void resetEnvelope(bool resetPlaybackPos = true) 
    { 
        envelopeState = EnvelopeState::Attack; 
        envelopeLevel = 0.0f; 
        isReleasing = false;
        if (resetPlaybackPos) {
            playbackPosition = 0; // Reset playback position to start of sample
        }
    }
    
    // Per-voice ADSR rates
    float getAttackRate() const { return voiceAttackRate; }
    float getDecayRate() const { return voiceDecayRate; }
    float getSustainLevel() const { return voiceSustainLevel; }
    float getReleaseRate() const { return voiceReleaseRate; }
    
    void setEnvelopeRates(float attack, float decay, float sustain, float release);
    
    // Update the envelope for this voice
    void updateEnvelope(int numSamples);
    
    // Process an audio block
    void processBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples, float masterVolume, float pan);
    
    // Trigger note off (start release phase)
    void noteOff();
    
    // Update envelope parameters
    void updateEnvelopeParameters(float attackTimeMs, float decayTimeMs, float sustainLevel, float releaseTimeMs);
    
    // Sample rate handling
    void setSampleRate(float sampleRate);
    float getSampleRate() const;
    
    // Check if the voice is active (not in idle state)
    bool isActive() const { return envelopeState != EnvelopeState::Idle; }
    
    // Check if the voice is finished (envelope in idle state)
    bool isFinished() const { return envelopeState == EnvelopeState::Idle; }
    
private:
    int playbackPosition = 0;
    float volume = 1.0f;
    float playbackRate = 1.0f;
    int cellX = -1;
    int cellY = -1;
    
    // Sample buffer handling
    const juce::AudioBuffer<float>* sampleBuffer = nullptr;
    
    // ADSR envelope state
    EnvelopeState envelopeState = EnvelopeState::Attack;
    float envelopeLevel = 0.0f;
    bool isReleasing = false;
    
    // Per-voice ADSR rates
    float voiceAttackRate = 0.0f;
    float voiceDecayRate = 0.0f;
    float voiceSustainLevel = 0.5f;
    float voiceReleaseRate = 0.0f;
    
    float currentSampleRate = 44100.0f;
};
