#!/bin/bash

# Ensure the script exits on any error
set -e

# Compiler and flags
CXX="clang++"
CXXFLAGS="-std=c++17 -Wall -Wextra"

# Include and library paths for macOS (using Homebrew paths for ARM64)
INCLUDES="-I/opt/homebrew/include"
LDFLAGS="-L/opt/homebrew/lib"
LIBS="-lglfw -lGLEW -framework OpenGL -lfftw3 -lsndfile -lportaudio"
FFMPEG_LIBS="-lavcodec -lavformat -lavutil -lswscale"

# Source files (alphabetized)
SOURCES=(
    "ascii_bar_equalizer.cpp"
    "balls_visualizer.cpp"
    "bar_equalizer.cpp"
    "mini_bar_equalizer.cpp"
    "cube_visualizer.cpp"
    "grid_visualizer.cpp"
    "hacker_terminal.cpp"
    "maze_visualizer.cpp"
    "mini_racer_visualizer.cpp"
    "mini_spectrogram.cpp"
    "multi_band_circle_waveform.cpp"
    "multi_band_waveform.cpp"
    "racer_visualizer.cpp"
    "scroller_text.cpp"
    "spectrogram.cpp"
    "terrain_visualizer_3d.cpp"
    "visualizer.cpp"
    "visualizer_factory.cpp"
    "waveform.cpp"
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
