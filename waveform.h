#ifndef WAVEFORM_H
#define WAVEFORM_H

#include "visualizer_base.h"
#include <vector>

class Waveform : public Visualizer {
public:
    Waveform();
    ~Waveform() override;

    // Add method to set multiple audio data sources
    void setAudioSources(const std::vector<std::vector<float>>& sources);

    // Implement the base class methods
    void renderFrame(const std::vector<float>& audioData, 
                   double* fftInputBuffer, 
                   fftw_complex* fftOutputBuffer,
                   fftw_plan& fftPlan, 
                   float timeSeconds) override;

    void renderLiveFrame(const std::vector<float>& audioData, 
                       double* fftInputBuffer, 
                       fftw_complex* fftOutputBuffer,
                       fftw_plan& fftPlan, 
                       size_t currentPosition) override;

private:
    const int N = 1024; // Number of samples to display
    std::vector<std::vector<float>> audioSources; // Multiple audio sources
    
    // Helper methods for multi-waveform layout
    void calculateGridDimensions(int numSources, int& rows, int& cols) const;
    void renderWaveform(const std::vector<float>& data, size_t position, 
                       float x1, float y1, float x2, float y2);
};

#endif // WAVEFORM_H 