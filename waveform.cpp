#include "waveform.h"
#include <algorithm>
#include <cmath>

Waveform::Waveform() {
}

Waveform::~Waveform() {
}

void Waveform::setAudioSources(const std::vector<std::vector<float>>& sources) {
    audioSources = sources;
}

void Waveform::calculateGridDimensions(int numSources, int& rows, int& cols) const {
    if (numSources <= 1) {
        rows = 1;
        cols = 1;
    } else if (numSources == 2) {
        rows = 1;
        cols = 2;
    } else if (numSources <= 4) {
        rows = 2;
        cols = 2;
    } else if (numSources <= 6) {
        rows = 2;
        cols = 3;
    } else {
        rows = 2;
        cols = 4;
    }
}

void Waveform::renderWaveform(const std::vector<float>& data, size_t position,
                            float x1, float y1, float x2, float y2) {
    // Set line width to 5 pixels for thicker waveform
    glLineWidth(5.0f);
    
    // Set color for visualization
    glColor3f(0.0f, 1.0f, 0.0f); // Green visualization
    
    glBegin(GL_LINE_STRIP);
    
    // Display a window of samples from the current position
    int sampleCount = std::min(N, static_cast<int>(data.size() - position));
    float width = x2 - x1;
    float height = y2 - y1;
    float centerY = y1 + height / 2.0f;
    
    for (int i = 0; i < sampleCount; i++) {
        float x = x1 + width * i / (float)(sampleCount - 1);
        float y = centerY + (data[position + i] * height * 0.4f); // Scale by 0.4 to prevent clipping
        glVertex2f(x, y);
    }
    
    glEnd();
    
    // Reset line width to default
    glLineWidth(1.0f);
}

void Waveform::renderFrame(const std::vector<float>& audioData, 
                          double* fftInputBuffer, 
                          fftw_complex* fftOutputBuffer,
                          fftw_plan& fftPlan, 
                          float timeSeconds) {
    // Mark unused parameters to silence compiler warnings
    (void)fftInputBuffer;
    (void)fftOutputBuffer;
    (void)fftPlan;
    (void)audioData; // We'll use audioSources instead
    
    // Calculate grid dimensions based on number of sources
    int rows, cols;
    calculateGridDimensions(audioSources.size(), rows, cols);
    
    // Calculate cell dimensions
    float cellWidth = 2.0f / cols;
    float cellHeight = 2.0f / rows;
    
    // Calculate the sample index for the current time
    size_t sampleIndex = static_cast<size_t>(timeSeconds * 44100); // Assuming 44.1kHz
    
    // Draw border around the entire window
    glLineWidth(1.0f);
    glColor3f(0.3f, 0.3f, 0.3f); // Gray color for borders
    glBegin(GL_LINE_LOOP);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();
    
    // Render each waveform in its grid cell
    for (size_t i = 0; i < audioSources.size() && i < 8; i++) {
        int row = i / cols;
        int col = i % cols;
        
        // Calculate cell boundaries
        float x1 = -1.0f + col * cellWidth;
        float y1 = 1.0f - (row + 1) * cellHeight;
        float x2 = x1 + cellWidth;
        float y2 = y1 + cellHeight;
        
        // Draw cell border
        glLineWidth(1.0f);
        glColor3f(0.3f, 0.3f, 0.3f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x1, y1);
        glVertex2f(x2, y1);
        glVertex2f(x2, y2);
        glVertex2f(x1, y2);
        glEnd();
        
        // Render waveform if we have data and haven't exceeded the sample count
        if (sampleIndex < audioSources[i].size()) {
            // Add small padding inside each cell
            float padding = 0.01f;
            renderWaveform(audioSources[i], sampleIndex,
                         x1 + padding, y1 + padding,
                         x2 - padding, y2 - padding);
        }
    }
}

void Waveform::renderLiveFrame(const std::vector<float>& audioData, 
                             double* fftInputBuffer, 
                             fftw_complex* fftOutputBuffer,
                             fftw_plan& fftPlan, 
                             size_t currentPosition) {
    // Mark unused parameters to silence compiler warnings
    (void)fftInputBuffer;
    (void)fftOutputBuffer;
    (void)fftPlan;
    (void)audioData; // We'll use audioSources instead
    
    // Calculate grid dimensions based on number of sources
    int rows, cols;
    calculateGridDimensions(audioSources.size(), rows, cols);
    
    // Calculate cell dimensions
    float cellWidth = 2.0f / cols;
    float cellHeight = 2.0f / rows;
    
    // Draw border around the entire window
    glLineWidth(1.0f);
    glColor3f(0.3f, 0.3f, 0.3f); // Gray color for borders
    glBegin(GL_LINE_LOOP);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();
    
    // Render each waveform in its grid cell
    for (size_t i = 0; i < audioSources.size() && i < 8; i++) {
        int row = i / cols;
        int col = i % cols;
        
        // Calculate cell boundaries
        float x1 = -1.0f + col * cellWidth;
        float y1 = 1.0f - (row + 1) * cellHeight;
        float x2 = x1 + cellWidth;
        float y2 = y1 + cellHeight;
        
        // Draw cell border
        glLineWidth(1.0f);
        glColor3f(0.3f, 0.3f, 0.3f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x1, y1);
        glVertex2f(x2, y1);
        glVertex2f(x2, y2);
        glVertex2f(x1, y2);
        glEnd();
        
        // Render waveform if we have data and haven't exceeded the sample count
        if (currentPosition < audioSources[i].size()) {
            // Add small padding inside each cell
            float padding = 0.01f;
            renderWaveform(audioSources[i], currentPosition,
                         x1 + padding, y1 + padding,
                         x2 - padding, y2 - padding);
        }
    }
} 