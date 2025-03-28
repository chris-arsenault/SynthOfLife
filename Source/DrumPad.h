#pragma once

#include <JuceHeader.h>
#include "Voice.h"
#include "EnvelopeProcessor.h"

class DrumPad
{
public:
    DrumPad();
    ~DrumPad();
    
    // Prepare for playback
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    
    // Release resources when playback stops
    void releaseResources();
    
    // Load a sample from a file
    void loadSample(const juce::File& file);
    
    // Trigger sample playback
    void triggerSample(float velocity);
    
    // Trigger sample with pitch shift
    void triggerSampleWithPitch(float velocity, int pitchShiftSemitones);
    
    // Trigger sample for a specific cell in the Game of Life grid
    void triggerSampleForCell(float velocity, int cellX, int cellY);
    
    // Trigger sample with pitch shift for a specific cell in the Game of Life grid
    void triggerSampleWithPitchForCell(float velocity, int pitchShiftSemitones, int cellX, int cellY);
    
    // Update pitch for a specific cell in the Game of Life grid without retriggering
    void updatePitchForCell(int pitchShiftSemitones, int cellX, int cellY);
    
    // Stop all sample playback immediately
    void stopSample();
    
    // Stop sample playback for a specific cell in the Game of Life grid
    void stopSampleForCell(int cellX, int cellY);
    
    // Process audio for this pad
    void processAudio(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    
    // Render audio to a buffer
    void renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    
    // Release a sample (for ADSR envelope)
    void releaseSample();
    
    // Set all ADSR parameters at once
    void setEnvelopeParameters(float attackTimeMs, float decayTimeMs, float sustainLevel, float releaseTimeMs);
    
    // Getters and setters
    bool isPlaying() const { return !activeVoices.empty(); }
    void setVolume(float newVolume) { volume = newVolume; }
    void setPan(float newPan) { pan = newPan; }
    void setMuted(bool isMuted) { muted = isMuted; }
    void setMidiNote(int note) { midiNote = note; }
    void setPolyphony(int count) { maxPolyphony = juce::jlimit(1, 16, count); }
    int getPolyphony() const { return maxPolyphony; }
    
    // ADSR getters and setters
    void setAttack(float attackTimeMs);
    void setDecay(float decayTimeMs);
    void setSustain(float sustainLevel);
    void setRelease(float releaseTimeMs);
    float getAttack() const;
    float getDecay() const;
    float getSustain() const;
    float getRelease() const;
    
    // Legato mode setters and getters
    void setLegatoMode(bool legato) { legatoMode = legato; }
    bool isLegatoMode() const { return legatoMode; }
    
    // Get the file path
    juce::String getFilePath() const { return filePath; }
    float getVolume() const { return volume; }
    float getPan() const { return pan; }
    bool isMuted() const { return muted; }
    int getMidiNote() const { return midiNote; }
    
    // Recent note information getters
    int getLastPlayedNote() const { return lastPlayedNote; }
    float getLastPlayedVelocity() const { return lastPlayedVelocity; }
    juce::String getLastPlayedNoteAsString() const;
    
    // Get the current volume level for visualization (considers ADSR envelope)
    float getCurrentVolumeLevel() const;
    
    const juce::AudioBuffer<float>* getSampleBuffer() const { return &sampleBuffer; }
    
private:
    juce::AudioBuffer<float> sampleBuffer;
    juce::String filePath; // Path to the loaded sample file
    std::vector<Voice> activeVoices;
    int maxPolyphony = 4; // Default to 4 voices
    float volume = 0.8f; // Default volume set to 0.8
    float pan = 0.0f;
    bool muted = false;
    int midiNote = 0;
    double currentSampleRate = 44100.0; // Store the current sample rate
    bool legatoMode = true; // Default to legato mode (current behavior)
    
    // Track most recently played note information
    int lastPlayedNote = 0;
    float lastPlayedVelocity = 0.0f;
    
    // ADSR envelope processor
    EnvelopeProcessor envelopeProcessor;
};
