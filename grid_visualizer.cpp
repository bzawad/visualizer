#include "grid_visualizer.h"
#include <GL/glew.h>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <GLFW/glfw3.h>

void GridVisualizer::initialize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
}

void GridVisualizer::setAudioSources(const std::vector<std::vector<float>>& sources) {
    audioSources = sources;
}

void GridVisualizer::calculateGridDimensions(int numSources, int& rows, int& cols) {
    if (numSources <= 1) {
        rows = 1;
        cols = 1;
    } else if (numSources <= 2) {
        rows = 1;
        cols = 2;
    } else if (numSources <= 4) {
        rows = 2;
        cols = 2;
    } else if (numSources <= 6) {
        rows = 2;
        cols = 3;
    } else if (numSources <= 9) {  // Support up to 9 files
        rows = 3;
        cols = 3;
    } else {
        // Cap at 9 files, using 3x3 grid
        rows = 3;
        cols = 3;
    }
}

void GridVisualizer::processAudioForFFT(const std::vector<float>& audioData, size_t position, double* fftInputBuffer) {
    // Apply Hanning window and copy data
    for (int i = 0; i < N; i++) {
        size_t index = (position + i) % audioData.size();
        double multiplier = 0.5 * (1 - cos(2 * M_PI * i / (N - 1)));
        fftInputBuffer[i] = audioData[index] * multiplier;
    }
}

void GridVisualizer::renderFrequencyGrid(const std::vector<float>& magnitudes, float x1, float y1, float x2, float y2) {
    float cellWidth = (x2 - x1) / GRID_SIZE;
    float cellHeight = (y2 - y1) / GRID_SIZE;
    
    // Calculate frequency bands
    std::vector<float> gridValues(GRID_SIZE * GRID_SIZE, 0.0f);
    float logMinFreq = log10(MIN_FREQ);
    float logMaxFreq = log10(MAX_FREQ);
    float logRange = logMaxFreq - logMinFreq;
    
    // Map FFT bins to grid cells
    for (int i = 1; i < N/2; i++) {
        float freq = i * SAMPLE_RATE / (float)N;
        if (freq < MIN_FREQ || freq > MAX_FREQ) continue;
        
        float logFreq = log10(freq);
        float normalizedFreq = (logFreq - logMinFreq) / logRange;
        
        // Map frequency to X coordinate (left to right = low to high frequency)
        int gridX = static_cast<int>(normalizedFreq * GRID_SIZE);
        
        // Get magnitude and normalize it
        float magnitude = magnitudes[i];
        float normalizedMagnitude = std::min(1.0f, magnitude * 20.0f);  // Increased amplification
        
        // Fill the entire column up to the magnitude level
        if (gridX >= 0 && gridX < GRID_SIZE) {
            for (int y = 0; y < GRID_SIZE; y++) {
                float cellMagnitude = y / static_cast<float>(GRID_SIZE);
                if (cellMagnitude <= normalizedMagnitude) {
                    gridValues[y * GRID_SIZE + gridX] = std::max(gridValues[y * GRID_SIZE + gridX], 
                        1.0f - (cellMagnitude / normalizedMagnitude));  // Gradient effect
                }
            }
        }
    }
    
    // Apply smoothing to fill gaps
    std::vector<float> smoothedValues = gridValues;
    for (int y = 1; y < GRID_SIZE - 1; y++) {
        for (int x = 1; x < GRID_SIZE - 1; x++) {
            float sum = 0.0f;
            int count = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    sum += gridValues[(y + dy) * GRID_SIZE + (x + dx)];
                    count++;
                }
            }
            smoothedValues[y * GRID_SIZE + x] = sum / count;
        }
    }
    
    // Render grid cells
    glBegin(GL_QUADS);
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            float value = smoothedValues[y * GRID_SIZE + x];
            
            // Use black to white color scheme
            float brightness = value;
            glColor3f(brightness, brightness, brightness);
            
            float cellX = x1 + x * cellWidth;
            float cellY = y1 + y * cellHeight;
            
            glVertex2f(cellX, cellY);
            glVertex2f(cellX + cellWidth, cellY);
            glVertex2f(cellX + cellWidth, cellY + cellHeight);
            glVertex2f(cellX, cellY + cellHeight);
        }
    }
    glEnd();
    
    // Draw grid lines
    glColor3f(0.3f, 0.3f, 0.3f);  // Dark gray grid lines
    glBegin(GL_LINES);
    for (int i = 0; i <= GRID_SIZE; i++) {
        float x = x1 + i * cellWidth;
        float y = y1 + i * cellHeight;
        
        // Vertical lines
        glVertex2f(x, y1);
        glVertex2f(x, y2);
        
        // Horizontal lines
        glVertex2f(x1, y);
        glVertex2f(x2, y);
    }
    glEnd();
}

// Multi-source methods
void GridVisualizer::renderFrame(const std::vector<std::vector<float>>& audioSources,
                               double* in,
                               fftw_complex* out,
                               fftw_plan& plan,
                               float timeSeconds) {
    int rows, cols;
    calculateGridDimensions(audioSources.size(), rows, cols);
    
    // Ensure we're using the full window space
    float gridWidth = 2.0f / cols;
    float gridHeight = 2.0f / rows;
    float padding = 0.01f;  // Smaller padding for better space utilization
    
    for (size_t i = 0; i < audioSources.size(); i++) {
        // Calculate row and column indices
        int row = i / cols;
        int col = i % cols;
        
        // Calculate grid position in OpenGL coordinates
        float x1 = -1.0f + col * gridWidth;
        float y1 = 1.0f - (row + 1) * gridHeight;
        float x2 = x1 + gridWidth;
        float y2 = y1 + gridHeight;
        
        // Add padding
        x1 += padding;
        y1 += padding;
        x2 -= padding;
        y2 -= padding;
        
        // Process and render the grid
        size_t position = static_cast<size_t>(timeSeconds * SAMPLE_RATE) % audioSources[i].size();
        processAudioForFFT(audioSources[i], position, in);
        
        fftw_execute(plan);
        
        std::vector<float> magnitudes(N/2);
        for (int j = 0; j < N/2; j++) {
            magnitudes[j] = sqrt(out[j][0] * out[j][0] + out[j][1] * out[j][1]) / N;
        }
        
        renderFrequencyGrid(magnitudes, x1, y1, x2, y2);
    }
}

void GridVisualizer::renderLiveFrame(const std::vector<std::vector<float>>& audioSources,
                                   double* in,
                                   fftw_complex* out,
                                   fftw_plan& plan,
                                   size_t currentPosition) {
    int rows, cols;
    calculateGridDimensions(audioSources.size(), rows, cols);
    
    // Ensure we're using the full window space
    float gridWidth = 2.0f / cols;
    float gridHeight = 2.0f / rows;
    float padding = 0.01f;  // Smaller padding for better space utilization
    
    for (size_t i = 0; i < audioSources.size(); i++) {
        // Calculate row and column indices
        int row = i / cols;
        int col = i % cols;
        
        // Calculate grid position in OpenGL coordinates
        float x1 = -1.0f + col * gridWidth;
        float y1 = 1.0f - (row + 1) * gridHeight;
        float x2 = x1 + gridWidth;
        float y2 = y1 + gridHeight;
        
        // Add padding
        x1 += padding;
        y1 += padding;
        x2 -= padding;
        y2 -= padding;
        
        // Process and render the grid
        processAudioForFFT(audioSources[i], currentPosition, in);
        
        fftw_execute(plan);
        
        std::vector<float> magnitudes(N/2);
        for (int j = 0; j < N/2; j++) {
            magnitudes[j] = sqrt(out[j][0] * out[j][0] + out[j][1] * out[j][1]) / N;
        }
        
        renderFrequencyGrid(magnitudes, x1, y1, x2, y2);
    }
}

// Legacy single-source methods
void GridVisualizer::renderFrame(const std::vector<float>& audioData,
                               double* in,
                               fftw_complex* out,
                               fftw_plan& plan,
                               float timeSeconds) {
    std::vector<std::vector<float>> sources = {audioData};
    renderFrame(sources, in, out, plan, timeSeconds);
}

void GridVisualizer::renderLiveFrame(const std::vector<float>& audioData,
                                   double* in,
                                   fftw_complex* out,
                                   fftw_plan& plan,
                                   size_t currentPosition) {
    std::vector<std::vector<float>> sources = {audioData};
    renderLiveFrame(sources, in, out, plan, currentPosition);
} 