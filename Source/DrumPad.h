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
    
    // Process audio for this pad
    void processAudio(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    
    // Getters and setters
    bool isPlaying() const { return playing; }
    void setVolume(float newVolume) { volume = newVolume; }
    void setPan(float newPan) { pan = newPan; }
    void setMuted(bool muted) { this->muted = muted; }
    void setMidiNote(int note) { midiNote = note; }
    
    const juce::AudioBuffer<float>* getSampleBuffer() const { return &sampleBuffer; }
    juce::String getFilePath() const { return filePath; }
    float getVolume() const { return volume; }
    float getPan() const { return pan; }
    bool isMuted() const { return muted; }
    int getMidiNote() const { return midiNote; }
    
private:
    juce::AudioBuffer<float> sampleBuffer;
    juce::String filePath;
    bool playing = false;
    int playbackPosition = 0;
    float volume = 1.0f;
    float pan = 0.0f;
    bool muted = false;
    int midiNote = 0;
    float playbackRate = 1.0f; // For pitch shifting
    double currentSampleRate = 44100.0; // Store the current sample rate
};
