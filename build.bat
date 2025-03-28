@echo off
echo Building Synth of Life VST Plugin...

echo Cleaning build directory...
if exist "build_new" (
    rmdir /s /q build_new
)

echo Creating build directory...
mkdir build_new

cd build_new

echo Running CMake configuration...
cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON

if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

echo Building with Visual Studio (Release)...
cmake --build . --config Release --verbose

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build complete!
echo VST3 plugin can be found in: build_new\SynthOfLife_artefacts\Release\VST3\Synth of Life.vst3
echo Standalone application can be found in: build_new\SynthOfLife_artefacts\Release\Standalone\Synth of Life.exe

pause
