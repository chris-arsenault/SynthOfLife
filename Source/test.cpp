#include <JuceHeader.h>

// Simple test function to verify that the build system is working
void testFunction()
{
    juce::String testString = "Test";
    juce::Logger::writeToLog(testString);
}
