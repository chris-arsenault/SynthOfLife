cmake_minimum_required(VERSION 3.15)

project(SynthOfLife VERSION 1.4.0)

# Include the JUCE CMake utilities
add_subdirectory(JUCE)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

# Initialize JUCE
juce_add_plugin(SynthOfLife
    VERSION 1.4.0
    COMPANY_NAME "Ahara"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE  # Changed to FALSE to avoid admin privileges issue
    PLUGIN_MANUFACTURER_CODE Ahar
    PLUGIN_CODE SoLf
    FORMATS VST3 Standalone
    PRODUCT_NAME "Synth of Life")

# Generate JuceHeader.h
juce_generate_juce_header(SynthOfLife)

# Add source files - including UI components
target_sources(SynthOfLife
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DrumPad.cpp
        Source/GameOfLife.cpp
        Source/ParameterManager.cpp
        Source/Voice.cpp
        Source/EnvelopeProcessor.cpp
        Source/Grid.cpp
        Source/UI/DrumPadComponent.cpp
        Source/UI/GameOfLifeComponent.cpp
        Source/UI/NoteActivityIndicator.cpp
        Source/UI/SampleSettingsComponent.cpp)

# Set include directories
target_include_directories(SynthOfLife
    PRIVATE
        Source
        Source/UI
        ${CMAKE_CURRENT_BINARY_DIR}/JuceLibraryCode)

# Link with JUCE modules
target_link_libraries(SynthOfLife
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_plugin_client
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
