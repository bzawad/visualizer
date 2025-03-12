# Music Visualizer

A simple music visualizer that plays a WAV file and displays a 16-bar frequency meter in green.

## Features

- Play mono WAV files
- Visualize frequencies in 16 green bars
- Press Escape to exit the visualizer

## Requirements

The following libraries are required:
- GLEW
- GLFW
- FFTW3
- libsndfile
- PortAudio

On macOS, you can install these using Homebrew:

```bash
brew install glew glfw fftw libsndfile portaudio
```

## Building

Run the build script:

```bash
chmod +x build.sh
./build.sh
```

## Usage

```bash
./visualizer [wav_file]
```

For example:

```bash
./visualizer sample.wav
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