#pragma once

#include <JuceHeader.h>

class DrumPad
{
public:
    DrumPad();
    ~DrumPad();
    
    // Prepare for playback
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    
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
    
    // Stop all sample playback immediately
    void stopSample();
    
    // Stop sample playback for a specific cell in the Game of Life grid
    void stopSampleForCell(int cellX, int cellY);
    
    // Process audio for this pad
    void processAudio(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    
    // Getters and setters
    bool isPlaying() const { return !activeVoices.empty(); }
    void setVolume(float newVolume) { volume = newVolume; }
    void setPan(float newPan) { pan = newPan; }
    void setMuted(bool isMuted) { muted = isMuted; }
    void setMidiNote(int note) { midiNote = note; }
    void setPolyphony(int count) { maxPolyphony = juce::jlimit(1, 16, count); }
    int getPolyphony() const { return maxPolyphony; }
    
    const juce::AudioBuffer<float>* getSampleBuffer() const { return &sampleBuffer; }
    juce::String getFilePath() const { return filePath; }
    juce::String getSamplePath() const { return samplePath; }
    float getVolume() const { return volume; }
    float getPan() const { return pan; }
    bool isMuted() const { return muted; }
    int getMidiNote() const { return midiNote; }
    
    void setSamplePath(const juce::String& path) { samplePath = path; }
    
private:
    struct Voice {
        int playbackPosition = 0;
        float volume = 1.0f;
        float playbackRate = 1.0f;
        int cellX = -1;
        int cellY = -1;
    };
    
    juce::AudioBuffer<float> sampleBuffer;
    juce::String filePath;
    juce::String samplePath;
    std::vector<Voice> activeVoices;
    int maxPolyphony = 4; // Default to 4 voices
    float volume = 1.0f;
    float pan = 0.0f;
    bool muted = false;
    int midiNote = 0;
    double currentSampleRate = 44100.0; // Store the current sample rate
};
