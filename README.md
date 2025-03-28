# Synth of Life

A creative VST3 audio generator plugin that combines Conway's Game of Life with sample playback, created by Tsonu using the vibe-coding method.

## Features

- Interactive Game of Life grid that responds to MIDI input
- Sample playback triggered by Game of Life cell activations
- MIDI note-controlled simulation - Game of Life only runs when notes are active
- Column mapping system to assign different samples to grid columns
- Velocity and pitch control modes for expressive sample playback
- Visual note activity indicator
- Waveform display for audio output visualization
- Available as VST3 and Standalone application

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
5. Experiment with different initial grid patterns and sample mappings

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
