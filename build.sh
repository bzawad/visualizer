#!/bin/bash

# Get Homebrew prefix location
BREW_PREFIX=$(brew --prefix)

# C++ compiler flags
CXXFLAGS="-std=c++11 -Wall -Wextra -O2 -I${BREW_PREFIX}/include"

# Linker flags for required libraries
LDFLAGS="-L${BREW_PREFIX}/lib -lglfw -lGLEW -framework OpenGL -lfftw3 -lsndfile -lportaudio"
FFMPEG_LIBS="-lavcodec -lavformat -lavutil -lswscale"

# Compile the visualizer components
echo "Compiling bar_equalizer.cpp..."
g++ $CXXFLAGS -c bar_equalizer.cpp -o bar_equalizer.o

echo "Compiling waveform.cpp..."
g++ $CXXFLAGS -c waveform.cpp -o waveform.o

echo "Compiling multi_band_waveform.cpp..."
g++ $CXXFLAGS -c multi_band_waveform.cpp -o multi_band_waveform.o

echo "Compiling multi_band_circle_waveform.cpp..."
g++ $CXXFLAGS -c multi_band_circle_waveform.cpp -o multi_band_circle_waveform.o

echo "Compiling ascii_bar_equalizer.cpp..."
g++ $CXXFLAGS -c ascii_bar_equalizer.cpp -o ascii_bar_equalizer.o

echo "Compiling spectrogram.cpp..."
g++ $CXXFLAGS -c spectrogram.cpp -o spectrogram.o

echo "Compiling terrain_visualizer_3d.cpp..."
g++ $CXXFLAGS -c terrain_visualizer_3d.cpp -o terrain_visualizer_3d.o

echo "Compiling grid_visualizer.cpp..."
g++ $CXXFLAGS -c grid_visualizer.cpp -o grid_visualizer.o

echo "Compiling visualizer_factory.cpp..."
g++ $CXXFLAGS -c visualizer_factory.cpp -o visualizer_factory.o

# Compile main file
echo "Compiling visualizer.cpp..."
g++ $CXXFLAGS -c visualizer.cpp -o visualizer.o

# Link all object files
echo "Linking..."
g++ visualizer.o bar_equalizer.o waveform.o multi_band_waveform.o multi_band_circle_waveform.o ascii_bar_equalizer.o spectrogram.o terrain_visualizer_3d.o grid_visualizer.o visualizer_factory.o -o visualizer $LDFLAGS $FFMPEG_LIBS

echo "Build complete. Run with: ./visualizer"
