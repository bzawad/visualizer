#include "grid_visualizer.h"
#include <GL/glew.h>
#include <cmath>
#include <algorithm>

GridVisualizer::GridVisualizer()
{
}

GridVisualizer::~GridVisualizer()
{
}

void GridVisualizer::setAudioSources(const std::vector<std::vector<float>>& sources) {
    audioSources = sources;
}

void GridVisualizer::processAudioForFFT(const std::vector<float>& audioData, size_t position, double* fftInputBuffer) {
    // Apply Hanning window and fill FFT input buffer
    for (int i = 0; i < N; i++) {
        if (position + i < audioData.size()) {
            // Apply Hanning window to reduce spectral leakage
            double window = 0.5 * (1.0 - cos(2.0 * M_PI * i / (N - 1)));
            fftInputBuffer[i] = audioData[position + i] * window;
        } else {
            fftInputBuffer[i] = 0.0;
        }
    }
}

void GridVisualizer::renderFrame(const std::vector<float>& audioData,
                               double* fftInputBuffer,
                               fftw_complex* fftOutputBuffer,
                               fftw_plan& fftPlan,
                               float timeSeconds)
{
    // Calculate the sample index for the current time
    size_t sampleIndex = static_cast<size_t>(timeSeconds * 44100); // Assuming 44.1kHz

    // Process audio data for FFT
    processAudioForFFT(audioData, sampleIndex, fftInputBuffer);

    // Execute FFT
    fftw_execute(fftPlan);

    // Calculate magnitudes for each frequency bin using logarithmic spacing
    std::vector<float> magnitudes(GRID_SIZE * GRID_SIZE);
    float logMinFreq = log10(MIN_FREQ);
    float logMaxFreq = log10(MAX_FREQ);
    float logStep = (logMaxFreq - logMinFreq) / (GRID_SIZE * GRID_SIZE);

    for (int i = 0; i < GRID_SIZE * GRID_SIZE; i++) {
        // Calculate frequency for this grid cell using logarithmic spacing
        float logFreq = logMinFreq + i * logStep;
        float freq = pow(10.0f, logFreq);
        
        // Convert frequency to FFT bin index
        int bin = static_cast<int>((freq * N) / 44100.0f);
        bin = std::min(bin, N/2 - 1); // Ensure we don't exceed Nyquist frequency
        
        // Calculate magnitude
        float magnitude = 0.0f;
        if (bin >= 0 && bin < N/2) {
            magnitude = std::sqrt(fftOutputBuffer[bin][0] * fftOutputBuffer[bin][0] +
                                fftOutputBuffer[bin][1] * fftOutputBuffer[bin][1]);
            
            // Apply frequency-dependent scaling with increased response
            float freqScaling = std::pow(static_cast<float>(bin) / (N/2), 0.3f) * 1.25f;
            magnitude *= freqScaling;
            
            // Normalize with increased brightness
            magnitude = std::min(1.0f, magnitude / 20.0f); // Reduced from 25.0f to 20.0f for more brightness
        }
        
        magnitudes[i] = magnitude;
    }

    // Render the grid
    renderGrid(magnitudes);
}

void GridVisualizer::renderLiveFrame(const std::vector<float>& audioData,
                                   double* fftInputBuffer,
                                   fftw_complex* fftOutputBuffer,
                                   fftw_plan& fftPlan,
                                   size_t currentPosition)
{
    // Process audio data for FFT
    processAudioForFFT(audioData, currentPosition, fftInputBuffer);

    // Execute FFT
    fftw_execute(fftPlan);

    // Calculate magnitudes for each frequency bin using logarithmic spacing
    std::vector<float> magnitudes(GRID_SIZE * GRID_SIZE);
    float logMinFreq = log10(MIN_FREQ);
    float logMaxFreq = log10(MAX_FREQ);
    float logStep = (logMaxFreq - logMinFreq) / (GRID_SIZE * GRID_SIZE);

    for (int i = 0; i < GRID_SIZE * GRID_SIZE; i++) {
        // Calculate frequency for this grid cell using logarithmic spacing
        float logFreq = logMinFreq + i * logStep;
        float freq = pow(10.0f, logFreq);
        
        // Convert frequency to FFT bin index
        int bin = static_cast<int>((freq * N) / 44100.0f);
        bin = std::min(bin, N/2 - 1); // Ensure we don't exceed Nyquist frequency
        
        // Calculate magnitude
        float magnitude = 0.0f;
        if (bin >= 0 && bin < N/2) {
            magnitude = std::sqrt(fftOutputBuffer[bin][0] * fftOutputBuffer[bin][0] +
                                fftOutputBuffer[bin][1] * fftOutputBuffer[bin][1]);
            
            // Apply frequency-dependent scaling with increased response
            float freqScaling = std::pow(static_cast<float>(bin) / (N/2), 0.3f) * 1.25f;
            magnitude *= freqScaling;
            
            // Normalize with increased brightness
            magnitude = std::min(1.0f, magnitude / 20.0f); // Reduced from 25.0f to 20.0f for more brightness
        }
        
        magnitudes[i] = magnitude;
    }

    // Render the grid
    renderGrid(magnitudes);
}

void GridVisualizer::renderGrid(const std::vector<float>& magnitudes) {
    // Calculate cell size
    float cellWidth = 2.0f / GRID_SIZE;
    float cellHeight = 2.0f / GRID_SIZE;
    
    // Draw grid cells
    glBegin(GL_QUADS);
    for (int i = 0; i < GRID_SIZE * GRID_SIZE; i++) {
        // Convert linear index to grid coordinates
        // Start from bottom-left, go up, then next column
        int col = i / GRID_SIZE;
        int row = i % GRID_SIZE;
        
        // Calculate cell boundaries
        float x1 = -1.0f + col * cellWidth;
        float y1 = -1.0f + row * cellHeight;
        float x2 = x1 + cellWidth;
        float y2 = y1 + cellHeight;
        
        // Set color based on magnitude (white with varying brightness)
        float brightness = magnitudes[i];
        glColor3f(brightness, brightness, brightness);
        
        // Draw cell
        glVertex2f(x1, y1);
        glVertex2f(x2, y1);
        glVertex2f(x2, y2);
        glVertex2f(x1, y2);
    }
    glEnd();
    
    // Draw grid lines
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_LINES);
    for (int i = 0; i <= GRID_SIZE; i++) {
        float pos = -1.0f + i * cellWidth;
        // Vertical lines
        glVertex2f(pos, -1.0f);
        glVertex2f(pos, 1.0f);
        // Horizontal lines
        glVertex2f(-1.0f, pos);
        glVertex2f(1.0f, pos);
    }
    glEnd();
} 