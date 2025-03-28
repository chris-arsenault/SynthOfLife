#include "SampleSettingsComponent.h"

SampleSettingsComponent::SampleSettingsComponent(ParameterManager& pm, int startIndex, int numSamps)
    : paramManager(pm), startSampleIndex(startIndex), numSamples(numSamps)
{
    // Initialize controls for each sample
    for (int i = 0; i < numSamples; ++i)
    {
        int sampleIndex = startSampleIndex + i;
        
        // Create a new SampleControls object
        auto* controls = new SampleControls();
        
        // Set up name label
        controls->nameLabel.setText("Sample " + juce::String(sampleIndex + 1), juce::dontSendNotification);
        controls->nameLabel.setFont(juce::Font(16.0f, juce::Font::bold));
        controls->nameLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(controls->nameLabel);
        
        // Set up load button
        controls->loadButton.setButtonText("Load Sample");
        controls->loadButton.setComponentID(juce::String(sampleIndex));
        controls->loadButton.addListener(this);
        addAndMakeVisible(controls->loadButton);
        
        // Set up filename label
        controls->filenameLabel.setText("No sample loaded", juce::dontSendNotification);
        controls->filenameLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(controls->filenameLabel);
        
        // Set up volume slider
        controls->volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        controls->volumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        controls->volumeSlider.setRange(0.0, 1.0, 0.01);
        controls->volumeSlider.setTextValueSuffix(" Vol");
        controls->volumeSlider.setValue(0.8, juce::dontSendNotification); // Set default volume to 0.8
        addAndMakeVisible(controls->volumeSlider);
        
        // Set up pan slider
        controls->panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        controls->panSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        controls->panSlider.setRange(-1.0, 1.0, 0.01);
        controls->panSlider.setTextValueSuffix(" Pan");
        controls->panSlider.setValue(0.0, juce::dontSendNotification); // Center pan by default
        addAndMakeVisible(controls->panSlider);
        
        // Set up MIDI note slider
        controls->midiNoteSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        controls->midiNoteSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        controls->midiNoteSlider.setRange(0, 127, 1);
        controls->midiNoteSlider.setTextValueSuffix(" Note");
        controls->midiNoteSlider.setValue(60, juce::dontSendNotification); // Default to middle C (MIDI note 60)
        addAndMakeVisible(controls->midiNoteSlider);
        
        // Set up polyphony slider
        controls->polyphonySlider.setSliderStyle(juce::Slider::LinearHorizontal);
        controls->polyphonySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        controls->polyphonySlider.setRange(1, 16, 1);
        controls->polyphonySlider.setTextValueSuffix(" Voices");
        controls->polyphonySlider.setValue(4, juce::dontSendNotification); // Default to 4 voices
        addAndMakeVisible(controls->polyphonySlider);
        
        // Set up control mode combobox
        controls->controlModeBox.addItem("Velocity", 1);
        controls->controlModeBox.addItem("Pitch", 2);
        controls->controlModeBox.setSelectedId(1, juce::dontSendNotification);
        addAndMakeVisible(controls->controlModeBox);
        
        // Set up mute button
        controls->muteButton.setButtonText("Mute");
        controls->muteButton.setToggleState(false, juce::dontSendNotification);
        addAndMakeVisible(controls->muteButton);
        
        // Set up legato mode button
        controls->legatoButton.setButtonText("Legato Mode");
        controls->legatoButton.setToggleState(true, juce::dontSendNotification); // Default to true (current behavior)
        addAndMakeVisible(controls->legatoButton);
        
        // Set up ADSR component
        controls->adsrComponent.addListener(this);
        // Set default ADSR values
        controls->adsrComponent.setValues(10.0f, 100.0f, 0.7f, 200.0f);
        addAndMakeVisible(controls->adsrComponent);
        
        // Create parameter attachments
        auto& apvts = paramManager.getAPVTS();
        
        controls->volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "volume_" + juce::String(sampleIndex), controls->volumeSlider);
            
        controls->panAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "pan_" + juce::String(sampleIndex), controls->panSlider);
            
        controls->muteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, "mute_" + juce::String(sampleIndex), controls->muteButton);
            
        controls->midiNoteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "midi_note_" + juce::String(sampleIndex), controls->midiNoteSlider);
            
        controls->polyphonyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "polyphony_" + juce::String(sampleIndex), controls->polyphonySlider);
            
        controls->controlModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, "control_mode_" + juce::String(sampleIndex), controls->controlModeBox);
            
        controls->legatoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, "legato_" + juce::String(sampleIndex), controls->legatoButton);
        
        // Connect ADSR controls to parameters
        controls->adsrComponent.connectToParameters(apvts, sampleIndex);
        
        // Add the controls to the array
        sampleControls.add(controls);
    }
}

SampleSettingsComponent::~SampleSettingsComponent()
{
    // OwnedArray will automatically delete all the SampleControls objects
}

void SampleSettingsComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    // Draw vertical dividers between sample columns
    g.setColour(juce::Colours::grey);
    for (int i = 1; i < numSamples; ++i)
    {
        int x = i * (getWidth() / numSamples);
        g.drawLine(x, 0, x, getHeight(), 1.0f);
    }
}

void SampleSettingsComponent::resized()
{
    // Calculate the width for each sample's controls
    int sampleWidth = getWidth() / numSamples;
    
    // Position controls for each sample
    for (int i = 0; i < numSamples; ++i)
    {
        auto* controls = sampleControls[i];
        int x = i * sampleWidth;
        
        // Layout for each sample's controls
        int margin = 10;
        int labelHeight = 24;
        int controlHeight = 24;
        int controlSpacing = 8;
        
        // Name label at the top
        controls->nameLabel.setBounds(x + margin, margin, sampleWidth - 2 * margin, labelHeight);
        
        // Controls below the label
        int controlY = margin + labelHeight + controlSpacing;
        
        // Load button and filename label
        controls->loadButton.setBounds(x + margin, controlY, 100, controlHeight);
        controls->filenameLabel.setBounds(x + margin, controlY + controlHeight + controlSpacing, sampleWidth - 2 * margin, controlHeight);
        controlY += 2 * controlHeight + 2 * controlSpacing;
        
        // Volume slider
        controls->volumeSlider.setBounds(x + margin, controlY, sampleWidth - 2 * margin, controlHeight);
        controlY += controlHeight + controlSpacing;
        
        // Pan slider
        controls->panSlider.setBounds(x + margin, controlY, sampleWidth - 2 * margin, controlHeight);
        controlY += controlHeight + controlSpacing;
        
        // MIDI note slider
        controls->midiNoteSlider.setBounds(x + margin, controlY, sampleWidth - 2 * margin, controlHeight);
        controlY += controlHeight + controlSpacing;
        
        // Polyphony slider
        controls->polyphonySlider.setBounds(x + margin, controlY, sampleWidth - 2 * margin, controlHeight);
        controlY += controlHeight + controlSpacing;
        
        // Control mode combobox
        controls->controlModeBox.setBounds(x + margin, controlY, sampleWidth - 2 * margin, controlHeight);
        controlY += controlHeight + controlSpacing;
        
        // Mute button
        controls->muteButton.setBounds(x + margin, controlY, sampleWidth - 2 * margin, controlHeight);
        controlY += controlHeight + controlSpacing;
        
        // Legato mode button
        controls->legatoButton.setBounds(x + margin, controlY, sampleWidth - 2 * margin, controlHeight);
        controlY += controlHeight + controlSpacing;
        
        // ADSR component - give it more height
        int adsrHeight = 250; // Increased from 220 to ensure all controls are visible
        controls->adsrComponent.setBounds(x + margin, controlY, sampleWidth - 2 * margin, adsrHeight);
    }
}

void SampleSettingsComponent::buttonClicked(juce::Button* button)
{
    // Check if we have drum pads set
    if (drumPads == nullptr)
        return;
    
    // Find which sample's load button was clicked
    for (int i = 0; i < numSamples; ++i)
    {
        auto* controls = sampleControls[i];
        if (button == &controls->loadButton)
        {
            int sampleIndex = startSampleIndex + i;
            
            // Open file chooser to load a sample
            auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
            fileChooser = std::make_unique<juce::FileChooser>("Select a sample to load...",
                                  juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                                  "*.wav;*.mp3;*.aiff");
                                  
            fileChooser->launchAsync(flags, [this, sampleIndex](const juce::FileChooser& chooser)
            {
                if (chooser.getResults().size() > 0)
                {
                    juce::File file = chooser.getResult();
                    loadSampleForPad(sampleIndex, file);
                }
            });
            
            break;
        }
    }
}

bool SampleSettingsComponent::isInterestedInFileDrag(const juce::StringArray& files)
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

void SampleSettingsComponent::filesDropped(const juce::StringArray& files, int x, int /*y*/)
{
    // Check if we have drum pads set
    if (drumPads == nullptr)
        return;
    
    // Find which sample area the file was dropped on based on x coordinate (column)
    int sampleWidth = getWidth() / numSamples;
    int sampleIndex = x / sampleWidth;
    
    // Adjust for the starting sample index
    sampleIndex += startSampleIndex;
    
    // Make sure the index is valid
    if (sampleIndex >= startSampleIndex && sampleIndex < startSampleIndex + numSamples)
    {
        // Load the first valid audio file
        for (auto& file : files)
        {
            juce::File f(file);
            if (f.hasFileExtension("wav;mp3;aiff"))
            {
                loadSampleForPad(sampleIndex, f);
                break;
            }
        }
    }
}

void SampleSettingsComponent::loadSampleForPad(int padIndex, const juce::File& file)
{
    if (drumPads != nullptr && padIndex >= startSampleIndex && padIndex < startSampleIndex + numSamples)
    {
        // Load the sample
        drumPads[padIndex].loadSample(file);
        
        // Update the filename label
        int localIndex = padIndex - startSampleIndex;
        if (localIndex >= 0 && localIndex < sampleControls.size())
        {
            sampleControls[localIndex]->filenameLabel.setText(file.getFileName(), juce::dontSendNotification);
            
            // Update the ADSR component with the current values from the DrumPad
            sampleControls[localIndex]->adsrComponent.setValues(
                drumPads[padIndex].getAttack(),
                drumPads[padIndex].getDecay(),
                drumPads[padIndex].getSustain(),
                drumPads[padIndex].getRelease()
            );
            
            // Update the legato mode button
            sampleControls[localIndex]->legatoButton.setToggleState(
                drumPads[padIndex].isLegatoMode(),
                juce::dontSendNotification
            );
        }
        
        // Debug output
        DBG("Loaded sample: " + file.getFullPathName() + " for pad " + juce::String(padIndex));
    }
}

void SampleSettingsComponent::updateADSRComponentsFromDrumPads()
{
    if (drumPads == nullptr)
        return;
        
    // Update ADSR components for each sample
    for (int i = 0; i < sampleControls.size(); ++i)
    {
        int padIndex = startSampleIndex + i;
        if (padIndex < ParameterManager::NUM_SAMPLES)
        {
            // Get ADSR values from the drum pad
            float attack = drumPads[padIndex].getAttack();
            float decay = drumPads[padIndex].getDecay();
            float sustain = drumPads[padIndex].getSustain();
            float release = drumPads[padIndex].getRelease();
            
            // Debug output
            DBG("Updating ADSR UI for pad " + juce::String(padIndex) + 
                ": A=" + juce::String(attack) + 
                ", D=" + juce::String(decay) + 
                ", S=" + juce::String(sustain) + 
                ", R=" + juce::String(release));
            
            // Update the ADSR component
            sampleControls[i]->adsrComponent.setValues(attack, decay, sustain, release);
            
            // Update the filename label
            juce::String path = drumPads[padIndex].getFilePath();
            if (path.isNotEmpty())
            {
                juce::File file(path);
                sampleControls[i]->filenameLabel.setText(file.getFileName(), juce::dontSendNotification);
            }
            else
            {
                sampleControls[i]->filenameLabel.setText("No sample loaded", juce::dontSendNotification);
            }
        }
    }
}

// ADSR envelope listener methods
void SampleSettingsComponent::attackChanged(float newValue)
{
    // Find which ADSR component triggered this
    for (int i = 0; i < sampleControls.size(); ++i)
    {
        auto* controls = sampleControls[i];
        if (&controls->adsrComponent == juce::Component::getCurrentlyFocusedComponent())
        {
            int padIndex = startSampleIndex + i;
            if (drumPads != nullptr && padIndex < ParameterManager::NUM_SAMPLES)
            {
                drumPads[padIndex].setAttack(newValue);
                DBG("Attack changed for pad " + juce::String(padIndex) + ": " + juce::String(newValue) + " ms");
            }
            break;
        }
    }
}

void SampleSettingsComponent::decayChanged(float newValue)
{
    // Find which ADSR component triggered this
    for (int i = 0; i < sampleControls.size(); ++i)
    {
        auto* controls = sampleControls[i];
        if (&controls->adsrComponent == juce::Component::getCurrentlyFocusedComponent())
        {
            int padIndex = startSampleIndex + i;
            if (drumPads != nullptr && padIndex < ParameterManager::NUM_SAMPLES)
            {
                drumPads[padIndex].setDecay(newValue);
                DBG("Decay changed for pad " + juce::String(padIndex) + ": " + juce::String(newValue) + " ms");
            }
            break;
        }
    }
}

void SampleSettingsComponent::sustainChanged(float newValue)
{
    // Find which ADSR component triggered this
    for (int i = 0; i < sampleControls.size(); ++i)
    {
        auto* controls = sampleControls[i];
        if (&controls->adsrComponent == juce::Component::getCurrentlyFocusedComponent())
        {
            int padIndex = startSampleIndex + i;
            if (drumPads != nullptr && padIndex < ParameterManager::NUM_SAMPLES)
            {
                drumPads[padIndex].setSustain(newValue);
                DBG("Sustain changed for pad " + juce::String(padIndex) + ": " + juce::String(newValue));
            }
            break;
        }
    }
}

void SampleSettingsComponent::releaseChanged(float newValue)
{
    // Find which ADSR component triggered this
    for (int i = 0; i < sampleControls.size(); ++i)
    {
        auto* controls = sampleControls[i];
        if (&controls->adsrComponent == juce::Component::getCurrentlyFocusedComponent())
        {
            int padIndex = startSampleIndex + i;
            if (drumPads != nullptr && padIndex < ParameterManager::NUM_SAMPLES)
            {
                drumPads[padIndex].setRelease(newValue);
                DBG("Release changed for pad " + juce::String(padIndex) + ": " + juce::String(newValue) + " ms");
            }
            break;
        }
    }
}
