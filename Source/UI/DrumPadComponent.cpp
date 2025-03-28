#include "DrumPadComponent.h"

DrumPadComponent::DrumPadComponent(ParameterManager& pm)
    : paramManager(pm)
{
    // Initialize controls for each drum pad
    for (int i = 0; i < ParameterManager::NUM_DRUM_PADS; ++i)
    {
        // Create load button
        padControls[i].loadButton = std::make_unique<juce::TextButton>("Load Sample");
        padControls[i].loadButton->setButtonText("Load Sample");
        padControls[i].loadButton->addListener(this);
        padControls[i].loadButton->setComponentID(juce::String(i));
        addAndMakeVisible(padControls[i].loadButton.get());
        
        // Create volume slider
        padControls[i].volumeSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
        padControls[i].volumeSlider->setRange(0.0, 1.0);
        padControls[i].volumeSlider->setValue(0.8);
        padControls[i].volumeSlider->setTextValueSuffix(" Vol");
        addAndMakeVisible(padControls[i].volumeSlider.get());
        
        // Create pan slider
        padControls[i].panSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
        padControls[i].panSlider->setRange(-1.0, 1.0);
        padControls[i].panSlider->setValue(0.0);
        padControls[i].panSlider->setTextValueSuffix(" Pan");
        addAndMakeVisible(padControls[i].panSlider.get());
        
        // Create mute button
        padControls[i].muteButton = std::make_unique<juce::ToggleButton>("Mute");
        padControls[i].muteButton->setToggleState(false, juce::dontSendNotification);
        addAndMakeVisible(padControls[i].muteButton.get());
        
        // Create filename label
        padControls[i].filenameLabel = std::make_unique<juce::Label>("Filename", "No sample loaded");
        padControls[i].filenameLabel->setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(padControls[i].filenameLabel.get());
        
        // Create parameter attachments
        padControls[i].volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            paramManager.getAPVTS(), "volume" + juce::String(i), *padControls[i].volumeSlider);
            
        padControls[i].panAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            paramManager.getAPVTS(), "pan" + juce::String(i), *padControls[i].panSlider);
            
        padControls[i].muteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            paramManager.getAPVTS(), "mute" + juce::String(i), *padControls[i].muteButton);
    }
}

DrumPadComponent::~DrumPadComponent()
{
    // Nothing to clean up
}

void DrumPadComponent::paint(juce::Graphics& g)
{
    // Fill background
    g.fillAll(juce::Colours::darkgrey);
    
    // Draw pad sections
    for (int i = 0; i < ParameterManager::NUM_DRUM_PADS; ++i)
    {
        // Draw pad section background
        juce::Rectangle<int> padArea = getLocalBounds().withHeight(getHeight() / ParameterManager::NUM_DRUM_PADS)
                                                      .translated(0, i * (getHeight() / ParameterManager::NUM_DRUM_PADS));
                                                      
        g.setColour(juce::Colours::grey);
        g.fillRect(padArea.reduced(2));
        
        // Draw pad number
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(14.0f, juce::Font::bold));
        g.drawText("Pad " + juce::String(i + 1), padArea.removeFromLeft(50), juce::Justification::centred);
    }
}

void DrumPadComponent::resized()
{
    // Calculate pad height
    int padHeight = getHeight() / ParameterManager::NUM_DRUM_PADS;
    
    for (int i = 0; i < ParameterManager::NUM_DRUM_PADS; ++i)
    {
        // Calculate pad area
        juce::Rectangle<int> padArea = getLocalBounds().withHeight(padHeight)
                                                      .translated(0, i * padHeight)
                                                      .reduced(5);
                                                      
        // Layout for pad controls
        auto row1 = padArea.removeFromTop(25);
        row1.removeFromLeft(50); // Space for pad number
        padControls[i].loadButton->setBounds(row1.removeFromLeft(100));
        padControls[i].filenameLabel->setBounds(row1.removeFromLeft(200));
        padControls[i].muteButton->setBounds(row1.removeFromRight(60));
        
        auto row2 = padArea.removeFromTop(25);
        row2.removeFromLeft(50); // Space for alignment
        padControls[i].volumeSlider->setBounds(row2.removeFromLeft(200));
        padControls[i].panSlider->setBounds(row2.removeFromLeft(200));
    }
}

void DrumPadComponent::buttonClicked(juce::Button* button)
{
    for (int i = 0; i < ParameterManager::NUM_DRUM_PADS; ++i)
    {
        if (button == padControls[i].loadButton.get())
        {
            // Open file chooser to load a sample
            auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
            fileChooser = std::make_unique<juce::FileChooser>("Select a sample to load...",
                                     juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                                     "*.wav;*.mp3;*.aiff");
                                     
            fileChooser->launchAsync(flags, [this, i](const juce::FileChooser& chooser)
            {
                if (chooser.getResults().size() > 0)
                {
                    juce::File file = chooser.getResult();
                    loadSampleForPad(i, file);
                }
            });
            
            break;
        }
    }
}

bool DrumPadComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    // Check if any of the files are audio files
    for (auto& file : files)
    {
        juce::File f(file);
        if (f.hasFileExtension("wav;mp3;aiff"))
            return true;
    }
    
    return false;
}

void DrumPadComponent::filesDropped(const juce::StringArray& files, int x, int y)
{
    // Calculate which pad the file was dropped on
    int padHeight = getHeight() / ParameterManager::NUM_DRUM_PADS;
    int padIndex = y / padHeight;
    
    if (padIndex >= 0 && padIndex < ParameterManager::NUM_DRUM_PADS)
    {
        // Load the first valid audio file
        for (auto& file : files)
        {
            juce::File f(file);
            if (f.hasFileExtension("wav;mp3;aiff"))
            {
                loadSampleForPad(padIndex, f);
                break;
            }
        }
    }
}

void DrumPadComponent::loadSampleForPad(int padIndex, const juce::File& file)
{
    if (drumPads != nullptr && padIndex >= 0 && padIndex < ParameterManager::NUM_DRUM_PADS)
    {
        // Load the sample
        drumPads[padIndex].loadSample(file);
        
        // Update the filename label
        padControls[padIndex].filenameLabel->setText(file.getFileName(), juce::dontSendNotification);
    }
}
