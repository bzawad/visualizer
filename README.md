# Music Visualizer

A powerful music visualizer that supports multiple WAV files with various visualization types. Perfect for analyzing and comparing audio files visually.

## Features

- Play and visualize multiple WAV files simultaneously (up to 8 files)
- Support for mono and stereo WAV files
- Multiple visualization types:
  - Bar Equalizer (`bars`): Classic frequency bars visualization
  - Waveform (`waveform`): Grid-based waveform display with support for multiple files
  - Multi-band Waveform (`multiband`): Frequency-separated waveform visualization
  - ASCII Bar Equalizer (`ascii`): Text-based frequency visualization
  - Spectrogram (`spectrogram`): Time-frequency heat map
  - Multi-band Circle (`circle`): Circular frequency visualization
  - 3D Terrain (`terrain`): Three-dimensional terrain-like visualization
- Record visualizations to MP4 video files
- Interactive visualization switching
- Real-time audio mixing for multiple files

## Requirements

The following libraries are required:
- GLEW
- GLFW
- FFTW3
- libsndfile
- PortAudio
- FFmpeg (libavcodec, libavformat, libavutil, libswscale)

On macOS, you can install these using Homebrew:

```bash
brew install glew glfw fftw libsndfile portaudio ffmpeg
```

## Building

Run the build script:

```bash
chmod +x build.sh
./build.sh
```

## Usage

```bash
./visualizer [--type <type>] [--record output.mp4] <wav_files...>
```

Visualization types (alphabetical):
- `ascii`: ASCII frequency bars
- `bars`: Classic frequency bars (default)
- `circle`: Multi-band circle visualization
- `cube`: 3D cube visualization
- `grid`: Grid-based visualization
- `multiband`: Multi-band waveform
- `racer`: Synthwave racer visualization
- `scroller`: Scrolling text visualization
- `spectrogram`: Spectrogram display
- `terrain`: 3D terrain visualization
- `waveform`: Grid-based waveform display

For waveform visualization with multiple files, the display is arranged in a grid layout:
- 1 file: 1x1 grid
- 2 files: 1x2 grid
- 3-4 files: 2x2 grid
- 5-6 files: 2x3 grid
- 7-8 files: 2x4 grid

Examples:

```bash
# Default visualization (frequency bars) with single file
./visualizer sample.wav

# Waveform visualization with multiple files
./visualizer --type waveform song1.wav song2.wav song3.wav

# Multi-band visualization with two files
./visualizer --type multiband track1.wav track2.wav

# Record multiple waveforms to video
./visualizer --type waveform --record output.mp4 song1.wav song2.wav song3.wav

# 3D terrain visualization
./visualizer --type terrain music.wav
```

## Creating Test Files

You can create test files using SoX:

```bash
# Install SoX
brew install sox

# Create a simple 440Hz sine wave
sox -n -r 44100 -c 1 sample.wav synth 10 sine 440 vol 0.5

# Create a more complex sample with multiple frequencies
sox -n -r 44100 -c 1 complex_sample.wav synth 10 sine 110 sine 220 sine 440 sine 880 sine 1760 vol 0.5

# Create stereo test file
sox -n -r 44100 -c 2 stereo_test.wav synth 10 sine 440 sine 880 remix 1 2
```

## Controls

- Press `V` to cycle through visualization types
- Press `Esc` to exit the visualizer 

## Multiple File Support

When playing multiple WAV files:
- All files must have the same sample rate (44.1kHz)
- Files can have different lengths - shorter files will be padded with silence
- Audio is automatically mixed with equal weighting
- Each file is displayed individually in the waveform visualization
- For other visualization types, the mixed audio is visualized

## Video Recording

When using the `--record` option, the visualizer will save both the visualization and mixed audio to an MP4 video file. The recording will automatically stop when the longest audio file finishes playing. The resulting video is encoded using H.264 at 30 frames per second with AAC audio, and will have a resolution of 800x600.

Note: Recording requires FFmpeg libraries to be installed. 