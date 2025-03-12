# Music Visualizer

A simple music visualizer that plays a WAV file and displays a frequency or waveform visualization in green.

## Features

- Play mono or stereo WAV files
- Multiple visualization types:
  - Frequency bars (16 green bars)
  - Waveform display
  - Multiband waveforms
  - ASCII frequency bars
  - Spectrogram
- Record visualizations to MP4 video files
- Press Escape to exit the visualizer

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
./visualizer [--type bars|waveform] [--record output.mp4] <wav_file>
```

Examples:

```bash
# Default visualization (frequency bars)
./visualizer sample.wav

# Waveform visualization
./visualizer --type waveform sample.wav 

# Explicitly select bar equalizer
./visualizer --type bars sample.wav

# Record bar visualization to a video file
./visualizer --record output.mp4 sample.wav

# Record waveform visualization to a video file
./visualizer --type waveform --record output.mp4 sample.wav
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
```

## Controls

- Press `Esc` to exit the visualizer 

## Video Recording

When using the `--record` option, the visualizer will save the visualization to an MP4 video file. The recording will automatically stop when the audio playback finishes. The resulting video is encoded using H.264 at 30 frames per second and will have the same resolution as the visualizer window (800x600).

Note: Recording requires FFmpeg libraries to be installed. 