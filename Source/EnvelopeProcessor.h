#pragma once

#include <JuceHeader.h>

/**
 * Handles ADSR envelope calculations and parameter management
 */
class EnvelopeProcessor
{
public:
    EnvelopeProcessor();
    ~EnvelopeProcessor() = default;
    
    // Set the attack time in milliseconds
    void setAttackTime(float attackTimeMs);
    
    // Set the decay time in milliseconds
    void setDecayTime(float decayTimeMs);
    
    // Set the sustain level (0.0 to 1.0)
    void setSustainLevel(float sustainLvl);
    
    // Set the release time in milliseconds
    void setReleaseTime(float releaseTimeMs);
    
    // Set the sample rate
    void setSampleRate(double sampleRate);
    
    // Get the attack time in milliseconds
    float getAttackTime() const { return attackTime; }
    
    // Get the decay time in milliseconds
    float getDecayTime() const { return decayTime; }
    
    // Get the sustain level (0.0 to 1.0)
    float getSustainLevel() const { return sustainLevel; }
    
    // Get the release time in milliseconds
    float getReleaseTime() const { return releaseTime; }
    
    // Get the attack rate (per sample)
    float getAttackRate() const { return attackRate; }
    
    // Get the decay rate (per sample)
    float getDecayRate() const { return decayRate; }
    
    // Get the release rate (per sample)
    float getReleaseRate() const { return releaseRate; }
    
    // Calculate envelope rates based on times
    void calculateEnvelopeRates();
    
private:
    // ADSR times (in milliseconds)
    float attackTime;
    float decayTime;
    float sustainLevel; // 0.0 to 1.0
    float releaseTime;
    
    // ADSR rates (per sample)
    float attackRate;
    float decayRate;
    float releaseRate;
    
    // Sample rate
    double currentSampleRate = 44100.0;
};
