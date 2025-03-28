# Synth of Life

A creative VST3 audio generator plugin that combines Conway's Game of Life with sample playback and MIDI-controlled synthesis. The plugin creates evolving, generative audio patterns based on cellular automata rules, providing a unique and interactive sound design tool for musical exploration.

## Features

- Interactive Game of Life grid that responds to MIDI input
- Sample playback triggered by Game of Life cell activations
- MIDI note-controlled simulation - Game of Life only runs when notes are active
- Continuous sound generation while notes are held
- Polyphonic sample playback with per-cell voice tracking
- Column mapping system to assign different samples to grid columns
- Velocity and pitch control modes for expressive sample playback
- Position-based pitch shifting for melodic patterns
- Musical scale-aware pitch mapping for harmonious output
- Independent ADSR envelopes for each voice
- True polyphony with proper voice management
- Visual note activity indicator
- Waveform display for audio output visualization
- Available as VST3 and Standalone application

## Changelog

### Version 1.2.0 (March 28, 2025)

#### Musical Enhancements
- **Scale-Aware Pitch Mapping**: Added support for multiple musical scales (Major, Minor, Harmonic Minor, Chromatic, Pentatonic, Blues)
- **Row-Based Pitch Offsets**: Implemented pitch offsets based on row position (-7 to +8 semitones)
- **Independent ADSR Envelopes**: Each voice now has its own ADSR envelope for more expressive sound design
- **Improved Polyphony**: Fixed polyphony to properly maintain voices until they naturally end or are evicted due to polyphony limits

#### Technical Improvements
- Added `getPitchOffsetForRow` method to calculate scale-appropriate pitch offsets
- Fixed sample rate handling for accurate ADSR timing across different host sample rates
- Completely rewrote cell-based voice triggering for better polyphony management
- Enhanced legato mode to properly track voices per cell

### Version 1.1.1 (March 28, 2025)

#### MIDI Handling Improvements
- **Improved Note Triggering Logic**: Refined rules for when cells trigger samples based on MIDI input
- **Smarter Polyphony**: Only new cells trigger samples when already playing, existing cells continue without retriggering
- **Better Note Transitions**: When changing notes while others are held, only pitch is updated without retriggering samples
- **Clean Note Release**: All samples stop properly when all MIDI notes are released

#### Technical Enhancements
- Added `updatePitchForCell` method to update pitch without retriggering samples
- Implemented `updatePitchOnly` method for smooth note transitions
- Optimized cell state tracking for more musical playback behavior
- Enhanced debug output for easier troubleshooting

### Version 1.1.0 (March 28, 2025)

#### Major Improvements
- **Fixed Sound Stopping Issue**: Sound now continues playing as long as a MIDI note is held
- **Enhanced MIDI Note Tracking**: Added support for tracking multiple active MIDI notes simultaneously
- **Improved Pitch Control**: Fixed pitch columns to properly modify pitch based on the row of the triggering cell
- **Cell-Based Polyphony**: Added per-cell voice tracking to ensure only the intended voices are stopped when cells are deactivated

#### Technical Enhancements
- Added `activeNotes` set to track all currently held MIDI notes
- Implemented cell-specific sample triggering and stopping
- Enhanced Game of Life simulation to continuously update while notes are held
- Improved debug output for easier troubleshooting
- Optimized voice management for better performance

## Building the Project

### Prerequisites

- CMake (3.15 or higher)
- C++ compiler with C++17 support (e.g., Visual Studio 2019 or later on Windows)
- JUCE framework

### Build Steps

1. Clone this repository
2. Open a terminal/command prompt in the project directory
3. Run the build script:
   ```
   .\build.bat
   ```
   Or manually build with CMake:
   ```
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```

4. The built VST3 plugin will be located in the `build_new\SynthOfLife_artefacts\Release\VST3\` directory
5. The standalone application can be found in `build_new\SynthOfLife_artefacts\Release\Standalone\` directory

## Usage

1. Load the VST3 plugin in your favorite DAW (Digital Audio Workstation)
2. Play MIDI notes to activate the Game of Life simulation
3. Configure column mappings to assign samples to different columns of the grid
4. Watch as the Game of Life cells trigger samples based on their activation patterns
5. Hold notes to create continuously evolving sound patterns
6. Experiment with different initial grid patterns and sample mappings
7. Try different musical scales to create harmonious melodic patterns

## Project Structure

- `Source/` - Contains all source code files
  - `PluginProcessor.h/cpp` - Audio processing and MIDI handling
  - `PluginEditor.h/cpp` - UI code
  - `GameOfLife.h/cpp` - Game of Life simulation
  - `DrumPad.h/cpp` - Sample playback engine
  - `ParameterManager.h/cpp` - Plugin parameter management
  - `UI/` - UI components
    - `NoteActivityIndicator.h/cpp` - Visual indicator for MIDI note activity
- `CMakeLists.txt` - CMake build configuration
- `build.bat` - Windows build script

## License

This project is licensed under the MIT License - see the LICENSE file for details.
