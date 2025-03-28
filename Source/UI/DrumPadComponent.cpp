#include "DrumPadComponent.h"

DrumPadComponent::DrumPadComponent(ParameterManager& pm)
    : paramManager(pm)
{
    // Initialize pad buttons and labels
    for (int i = 0; i < ParameterManager::NUM_SAMPLES; ++i)
    {
        // Create pad button
        pads[i].padButton = std::make_unique<juce::TextButton>("Pad " + juce::String(i + 1));
        pads[i].padButton->setButtonText("Pad " + juce::String(i + 1));
        pads[i].padButton->setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        pads[i].padButton->setColour(juce::TextButton::buttonOnColourId, juce::Colours::green);
        pads[i].padButton->addListener(this);
        addAndMakeVisible(pads[i].padButton.get());
        
        // Create info label
        pads[i].infoLabel = std::make_unique<juce::Label>("Info", "Note: - | Vel: -");
        pads[i].infoLabel->setJustificationType(juce::Justification::centred);
        pads[i].infoLabel->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        pads[i].infoLabel->setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(pads[i].infoLabel.get());
        
        // Initialize volume history
        pads[i].resetVolumeHistory();
    }
    
    // Start the timer to update pad information
    startTimerHz(30); // Update 30 times per second
}

DrumPadComponent::~DrumPadComponent()
{
    stopTimer();
}

void DrumPadComponent::paint(juce::Graphics& g)
{
    // Fill the background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    // Draw grid lines
    g.setColour(juce::Colours::grey);
    
    int numRows = 4;
    int numCols = 4;
    
    float rowHeight = getHeight() / static_cast<float>(numRows);
    float colWidth = getWidth() / static_cast<float>(numCols);
    
    // Draw horizontal grid lines
    for (int row = 1; row < numRows; ++row)
    {
        float y = row * rowHeight;
        g.drawLine(0.0f, y, static_cast<float>(getWidth()), y);
    }
    
    // Draw vertical grid lines
    for (int col = 1; col < numCols; ++col)
    {
        float x = col * colWidth;
        g.drawLine(x, 0.0f, x, static_cast<float>(getHeight()));
    }
    
    // Draw volume graphs for each pad
    if (drumPads != nullptr)
    {
        for (int i = 0; i < ParameterManager::NUM_SAMPLES; ++i)
        {
            int row = i / 4;
            int col = i % 4;
            
            juce::Rectangle<int> padBounds(
                static_cast<int>(col * colWidth),
                static_cast<int>(row * rowHeight),
                static_cast<int>(colWidth),
                static_cast<int>(rowHeight)
            );
            
            // Draw the volume graph
            drawVolumeGraph(g, pads[i], padBounds);
        }
    }
}

void DrumPadComponent::resized()
{
    // Calculate grid dimensions
    int numRows = 4;
    int numCols = 4;
    
    float rowHeight = getHeight() / static_cast<float>(numRows);
    float colWidth = getWidth() / static_cast<float>(numCols);
    
    // Set up the pad buttons and info labels
    for (int i = 0; i < ParameterManager::NUM_SAMPLES; ++i)
    {
        int row = i / 4;
        int col = i % 4;
        
        // Calculate the bounds for this pad
        juce::Rectangle<int> padBounds(
            static_cast<int>(col * colWidth),
            static_cast<int>(row * rowHeight),
            static_cast<int>(colWidth),
            static_cast<int>(rowHeight)
        );
        
        // Button takes up the top 40% of the pad
        juce::Rectangle<int> buttonBounds = padBounds.withHeight(padBounds.getHeight() * 0.4f);
        
        // Info label takes up the bottom 20% of the pad
        juce::Rectangle<int> labelBounds = padBounds.withTop(padBounds.getBottom() - padBounds.getHeight() * 0.2f);
        
        // The volume graph will be drawn in the middle 40% (this is handled in the paint method)
        
        // Set the bounds for the button and label
        pads[i].padButton->setBounds(buttonBounds.reduced(2));
        pads[i].infoLabel->setBounds(labelBounds.reduced(2));
    }
}

void DrumPadComponent::buttonClicked(juce::Button* button)
{
    // Find which pad button was clicked
    for (int i = 0; i < ParameterManager::NUM_SAMPLES; ++i)
    {
        if (button == pads[i].padButton.get() && drumPads != nullptr)
        {
            // Trigger the sample with a velocity of 1.0 (maximum)
            drumPads[i].triggerSample(1.0f);
            
            // Update the pad info immediately
            pads[i].isPlaying = true;
            pads[i].lastVelocity = 1.0f;
            pads[i].lastPitch = drumPads[i].getMidiNote();
            pads[i].padColor = getColorForPad(1.0f, pads[i].lastPitch);
            pads[i].statusText = "Note: " + juce::String(pads[i].lastPitch) + " | Vel: 1.00";
            
            // Update the UI
            pads[i].padButton->setColour(juce::TextButton::buttonColourId, pads[i].padColor);
            pads[i].infoLabel->setText(pads[i].statusText, juce::dontSendNotification);
            
            break;
        }
    }
}

void DrumPadComponent::timerCallback()
{
    updatePadInfo();
}

void DrumPadComponent::updatePadInfo()
{
    if (drumPads == nullptr)
        return;
    
    for (int i = 0; i < ParameterManager::NUM_SAMPLES; ++i)
    {
        bool wasPlaying = pads[i].isPlaying;
        pads[i].isPlaying = drumPads[i].isPlaying();
        
        // Get the sample filename (without path and extension)
        juce::String sampleName = juce::File(drumPads[i].getFilePath()).getFileNameWithoutExtension();
        if (sampleName.isEmpty())
            sampleName = "No Sample";
        
        // Truncate sample name if too long (max 10 characters)
        if (sampleName.length() > 10)
            sampleName = sampleName.substring(0, 8) + "..";
        
        // Update the pad button text with the sample name
        pads[i].padButton->setButtonText(sampleName);
        
        // Get the most recently played note and velocity
        juce::String noteStr = drumPads[i].getLastPlayedNoteAsString();
        int velocityInt = static_cast<int>(drumPads[i].getLastPlayedVelocity() * 127.0f); // Convert to 0-127 scale
        
        // Update the pad information if the playing state has changed
        if (pads[i].isPlaying != wasPlaying || pads[i].isPlaying)
        {
            if (pads[i].isPlaying)
            {
                // Pad is playing, update with current information
                pads[i].padColor = juce::Colours::red;
                pads[i].statusText = "Note: " + noteStr + " | Vel: " + juce::String(velocityInt);
                
                // If the pad just started playing (wasn't playing before), reset the volume history
                if (!wasPlaying)
                {
                    pads[i].resetVolumeHistory();
                }
            }
            else
            {
                // Pad is not playing, reset to default state but keep the last note/velocity info
                pads[i].padColor = juce::Colours::darkgrey;
                pads[i].statusText = "Note: " + noteStr + " | Vel: " + juce::String(velocityInt);
            }
            
            // Update the UI
            pads[i].padButton->setColour(juce::TextButton::buttonColourId, pads[i].padColor);
            pads[i].infoLabel->setText(pads[i].statusText, juce::dontSendNotification);
        }
        
        // Update the volume history with the current volume level
        float currentVolume = drumPads[i].getCurrentVolumeLevel();
        pads[i].addVolumeToHistory(currentVolume);
    }
    
    repaint();
}

juce::Colour DrumPadComponent::getColorForPad(float velocity, int pitch)
{
    // Map velocity to brightness (0.0-1.0)
    float brightness = 0.5f + (velocity * 0.5f);
    
    // Map pitch to hue (0.0-1.0)
    // We'll use a simple mapping from MIDI note 0-127 to hue 0.0-1.0
    float hue = (pitch % 12) / 12.0f;
    
    // Create a color with the calculated hue, full saturation, and brightness based on velocity
    return juce::Colour::fromHSV(hue, 1.0f, brightness, 1.0f);
}

void DrumPadComponent::drawVolumeGraph(juce::Graphics& g, const PadInfo& pad, juce::Rectangle<int> bounds)
{
    // Define the graph area (middle 40% of the pad)
    float buttonHeight = bounds.getHeight() * 0.4f;
    float labelHeight = bounds.getHeight() * 0.2f;
    float graphHeight = bounds.getHeight() * 0.4f;
    
    juce::Rectangle<int> graphArea(
        bounds.getX(),
        bounds.getY() + static_cast<int>(buttonHeight),
        bounds.getWidth(),
        static_cast<int>(graphHeight)
    );
    
    // Draw background for the graph
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRect(graphArea);
    
    // If there's no volume history, just return
    if (!pad.historyFilled && pad.historyIndex == 0)
        return;
    
    // Draw the volume graph
    g.setColour(juce::Colours::green);
    
    // Calculate the width of each sample in the graph
    float sampleWidth = static_cast<float>(graphArea.getWidth()) / static_cast<float>(PadInfo::maxHistorySize);
    
    // Create a path for the volume graph
    juce::Path volumePath;
    bool pathStarted = false;
    
    // Start from the oldest sample
    int startIndex = pad.historyFilled ? pad.historyIndex : 0;
    int numSamples = pad.historyFilled ? PadInfo::maxHistorySize : pad.historyIndex;
    
    for (int i = 0; i < numSamples; ++i)
    {
        int index = (startIndex + i) % PadInfo::maxHistorySize;
        float volume = pad.volumeHistory[index];
        
        // Calculate the x and y positions
        float x = graphArea.getX() + i * sampleWidth;
        float y = graphArea.getBottom() - volume * graphArea.getHeight();
        
        if (!pathStarted)
        {
            volumePath.startNewSubPath(x, y);
            pathStarted = true;
        }
        else
        {
            volumePath.lineTo(x, y);
        }
    }
    
    // Draw the path with a stroke
    g.strokePath(volumePath, juce::PathStrokeType(1.5f));
    
    // Draw a line at the current volume level
    float currentVolume = pad.volumeHistory[pad.historyIndex > 0 ? pad.historyIndex - 1 : (pad.historyFilled ? PadInfo::maxHistorySize - 1 : 0)];
    float currentY = graphArea.getBottom() - currentVolume * graphArea.getHeight();
    
    g.setColour(juce::Colours::yellow);
    g.drawLine(
        static_cast<float>(graphArea.getRight() - 2),
        currentY,
        static_cast<float>(graphArea.getRight()),
        currentY,
        2.0f
    );
}
