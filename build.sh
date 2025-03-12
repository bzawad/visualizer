#!/bin/bash

# Get Homebrew prefix
BREW_PREFIX=$(brew --prefix)

# Set compiler flags
CXX=g++
CXXFLAGS="-Wall -Wextra -O2 -I${BREW_PREFIX}/include"
LIBS="-L${BREW_PREFIX}/lib -lglfw -lGLEW -framework OpenGL -lfftw3 -lsndfile -lportaudio"

# Output binary name
OUTPUT="visualizer"

# Source file(s)
SRC="visualizer.cpp"

# Compile
echo "Compiling $SRC..."
$CXX $SRC -o $OUTPUT $CXXFLAGS $LIBS

echo "Build complete. Run with: ./$OUTPUT"
