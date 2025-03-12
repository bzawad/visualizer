#include "waveform.h"
#include <algorithm>

Waveform::Waveform() {
}

Waveform::~Waveform() {
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
    
    // Set line width to 5 pixels for thicker waveform
    glLineWidth(5.0f);
    
    // Set color for visualization
    glColor3f(0.0f, 1.0f, 0.0f); // Green visualization
    
    // Calculate the sample index for the current time
    size_t sampleIndex = static_cast<size_t>(timeSeconds * 44100); // Assuming 44.1kHz
    if (sampleIndex >= audioData.size()) return;
    
    glBegin(GL_LINE_STRIP);
    
    // Display a window of samples from the current position
    int sampleCount = std::min(N, static_cast<int>(audioData.size() - sampleIndex));
    
    for (int i = 0; i < sampleCount; i++) {
        float x = -1.0f + 2.0f * i / (float)(sampleCount - 1);
        float y = audioData[sampleIndex + i] * 0.8f; // Scale to prevent clipping
        glVertex2f(x, y);
    }
    
    glEnd();
    
    // Reset line width to default
    glLineWidth(1.0f);
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
    
    // Set line width to 5 pixels for thicker waveform
    glLineWidth(5.0f);
    
    // Set color for visualization
    glColor3f(0.0f, 1.0f, 0.0f); // Green visualization
    
    glBegin(GL_LINE_STRIP);
    
    // Display a window of samples from the current position
    int sampleCount = std::min(N, static_cast<int>(audioData.size() - currentPosition));
    
    for (int i = 0; i < sampleCount; i++) {
        float x = -1.0f + 2.0f * i / (float)(sampleCount - 1);
        float y = audioData[currentPosition + i] * 0.8f; // Scale to prevent clipping
        glVertex2f(x, y);
    }
    
    glEnd();
    
    // Reset line width to default
    glLineWidth(1.0f);
} 