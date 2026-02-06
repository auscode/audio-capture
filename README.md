# Audio Capture Project

A native C++ Windows audio capture application that records both speaker output and microphone input simultaneously to separate WAV files using WASAPI.

## Features

- **Speaker Capture**: Records system audio using WASAPI loopback mode
- **Microphone Capture**: Records microphone input using standard WASAPI capture
- **Separate Files**: Saves audio to `output/speaker.wav` and `output/mic.wav`
- **No Audio Mixing**: Pure OS-level separation, no acoustic contamination
- **Professional Architecture**: Modular C++ design with proper RAII and threading

## Technology Stack

- **Language**: Modern C++17
- **Audio API**: Windows Audio Session API (WASAPI)
- **Device Management**: MMDevice API
- **Build System**: CMake
- **File Format**: WAV/PCM
- **Threading**: Dedicated capture threads for stability

## Requirements

- Windows 10 or later
- Visual Studio 2019+ or MinGW-w64
- CMake 3.16+
- C++17 compatible compiler

## Build Instructions

### Using Visual Studio

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -G "Visual Studio 16 2019" -A x64

# Build the project
cmake --build . --config Release

# Run the executable
.\Release\audio-capture.exe
```

### Using MinGW-w64

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -G "MinGW Makefiles"

# Build the project
cmake --build .

# Run the executable
.\audio-capture.exe
```

## Usage

1. Run the executable
2. The application will capture audio for 30 seconds
3. Files are saved to:
   - `output/speaker.wav` - System speaker output
   - `output/mic.wav` - Microphone input
4. Press Ctrl+C to stop early

## Architecture

### Core Components

- **AudioCapture**: Base class for audio capture functionality
- **LoopbackCapture**: Implements speaker audio capture using WASAPI loopback
- **MicCapture**: Implements microphone audio capture
- **WavWriter**: Handles WAV file writing with proper headers
- **Utils**: Platform utilities and helper functions

### Threading Model

- Thread 1: Speaker capture (LoopbackCapture)
- Thread 2: Microphone capture (MicCapture)
- Main thread: Orchestration and timing
- Lock-free audio buffer handling

### Audio Isolation

The key to this implementation is OS-level audio isolation:

- **Loopback capture** taps the digital render stream before it reaches speakers
- **Microphone capture** reads directly from the capture endpoint
- No acoustic path or mixing between streams
- Proper separation guaranteed by Windows audio subsystem

## Validation

### Testing Audio Separation

1. Play audio through speakers (YouTube, music player, etc.)
2. Speak into microphone simultaneously
3. Verify that:
   - `speaker.wav` contains only system audio
   - `mic.wav` contains only microphone input
   - No crossover between channels

### Tools for Verification

- **Audacity**: Load both WAV files to visualize and analyze
- **Silent Mic Test**: Unplug mic and verify `mic.wav` is silent
- **Headphones Test**: Use headphones to eliminate acoustic leakage

## File Structure

```
audio-capture/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── AudioCapture.h
│   ├── LoopbackCapture.h
│   ├── MicCapture.h
│   ├── WavWriter.h
│   └── Utils.h
├── src/
│   ├── main.cpp
│   ├── AudioCapture.cpp
│   ├── LoopbackCapture.cpp
│   ├── MicCapture.cpp
│   ├── WavWriter.cpp
│   └── Utils.cpp
└── output/
    ├── speaker.wav
    └── mic.wav
```

## Troubleshooting

### Common Issues

1. **No Audio in Files**: Check audio permissions and device availability
2. **Compilation Errors**: Ensure Windows SDK is properly installed
3. **Access Denied**: Run with administrator privileges if needed
4. **Device Not Found**: Verify audio devices are enabled in Windows

### Debug Tips

- Check Windows audio device manager
- Verify default playback/recording devices
- Test with different audio formats
- Monitor console output for error messages

## Technical Notes

- Uses **RAII** for COM object management
- **Deterministic cleanup** with proper resource handling
- **Thread-safe** audio capture with proper synchronization
- **Error handling** with detailed error messages
- **Cross-platform considerations** in utility functions

## Performance Characteristics

- Low latency audio capture (~10ms buffer)
- Minimal CPU overhead
- Memory efficient with streaming writes
- No audio compression or processing artifacts