#!/bin/bash

# Ensure the script exits on any error
set -e

# Compiler and flags
CXX="clang++"
CXXFLAGS="-std=c++17 -Wall -Wextra"

# Include and library paths for macOS (using Homebrew paths)
INCLUDES="-I/opt/homebrew/include"
LDFLAGS="-L/opt/homebrew/lib"
LIBS="-lglfw -lGLEW -framework OpenGL -lfftw3 -lsndfile -lportaudio"
FFMPEG_LIBS="-lavcodec -lavformat -lavutil -lswscale"

# Source files
SOURCES=(
    "bar_equalizer.cpp"
    "waveform.cpp"
    "multi_band_waveform.cpp"
    "multi_band_circle_waveform.cpp"
    "ascii_bar_equalizer.cpp"
    "spectrogram.cpp"
    "terrain_visualizer_3d.cpp"
    "grid_visualizer.cpp"
    "visualizer_factory.cpp"
    "visualizer.cpp"
    "scroller_text.cpp"
)

# Compile each source file
for source in "${SOURCES[@]}"; do
    echo "Compiling ${source}..."
    $CXX $CXXFLAGS $INCLUDES -c "$source"
done

# Link everything together
echo "Linking..."
$CXX $CXXFLAGS *.o $LDFLAGS $LIBS $FFMPEG_LIBS -o visualizer

echo "Build complete. Run with: ./visualizer"
