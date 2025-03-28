#include "EnvelopeProcessor.h"

EnvelopeProcessor::EnvelopeProcessor()
    : attackTime(10.0f),      // 10ms attack time
      decayTime(100.0f),      // 100ms decay time
      sustainLevel(0.7f),     // 70% sustain level
      releaseTime(200.0f),    // 200ms release time
      attackRate(0.0f),
      decayRate(0.0f),
      releaseRate(0.0f),
      currentSampleRate(44100.0)
{
    // Calculate initial envelope rates
    calculateEnvelopeRates();
}

void EnvelopeProcessor::setAttackTime(float attackTimeMs)
{
    attackTime = attackTimeMs;
    calculateEnvelopeRates();
}

void EnvelopeProcessor::setDecayTime(float decayTimeMs)
{
    decayTime = decayTimeMs;
    calculateEnvelopeRates();
}

void EnvelopeProcessor::setSustainLevel(float sustainLvl)
{
    sustainLevel = sustainLvl;
    calculateEnvelopeRates();
}

void EnvelopeProcessor::setReleaseTime(float releaseTimeMs)
{
    releaseTime = releaseTimeMs;
    calculateEnvelopeRates();
}

void EnvelopeProcessor::setSampleRate(double sampleRate)
{
    currentSampleRate = sampleRate;
    calculateEnvelopeRates();
}

void EnvelopeProcessor::calculateEnvelopeRates()
{
    // Convert milliseconds to per-sample increment rates
    
    // For attack: we need to go from 0 to 1 over attackTime milliseconds
    // For a 2000ms attack, we should increment by 1/(2000ms * sampleRate/1000) per sample
    float safeAttackTime = juce::jmax(1.0f, attackTime);
    attackRate = 1.0f / (safeAttackTime * 0.001f * static_cast<float>(currentSampleRate));
    
    // For decay: we need to go from 1 to sustainLevel over decayTime milliseconds
    float safeDecayTime = juce::jmax(1.0f, decayTime);
    decayRate = (1.0f - sustainLevel) / (safeDecayTime * 0.001f * static_cast<float>(currentSampleRate));
    
    // For release: we need to go from sustainLevel to 0 over releaseTime milliseconds
    float safeReleaseTime = juce::jmax(1.0f, releaseTime);
    releaseRate = sustainLevel / (safeReleaseTime * 0.001f * static_cast<float>(currentSampleRate));
    
    // Debug output to verify calculations
    DBG("ADSR Rates calculated - Attack: " + juce::String(attackRate) + 
        ", Decay: " + juce::String(decayRate) + 
        ", Sustain: " + juce::String(sustainLevel) + 
        ", Release: " + juce::String(releaseRate) +
        " (from A=" + juce::String(attackTime) + 
        "ms, D=" + juce::String(decayTime) + 
        "ms, S=" + juce::String(sustainLevel) + 
        ", R=" + juce::String(releaseTime) + "ms)");
}
