#pragma once

#include "visualizer_base.h"
#include <vector>
#include <fftw3.h>

class GridVisualizer : public Visualizer
{
public:
    GridVisualizer() = default;
    virtual ~GridVisualizer() = default;

    void initialize(int width, int height) override;
    
    // Multi-source methods
    void renderFrame(const std::vector<std::vector<float>>& audioSources,
                    double* in,
                    fftw_complex* out,
                    fftw_plan& plan,
                    float timeSeconds) override;
                    
    void renderLiveFrame(const std::vector<std::vector<float>>& audioSources,
                        double* in,
                        fftw_complex* out,
                        fftw_plan& plan,
                        size_t currentPosition) override;
    
    // Legacy single-source methods
    void renderFrame(const std::vector<float>& audioData,
                    double* in,
                    fftw_complex* out,
                    fftw_plan& plan,
                    float timeSeconds) override;
                    
    void renderLiveFrame(const std::vector<float>& audioData,
                        double* in,
                        fftw_complex* out,
                        fftw_plan& plan,
                        size_t currentPosition) override;
    
    void setAudioSources(const std::vector<std::vector<float>>& sources);

private:
    void calculateGridDimensions(int numSources, int& rows, int& cols);
    void processAudioForFFT(const std::vector<float>& audioData, size_t position, double* fftInputBuffer);
    void renderFrequencyGrid(const std::vector<float>& magnitudes, float x1, float y1, float x2, float y2);
    std::vector<std::vector<float>> audioSources;
    static const int GRID_SIZE = 32;
    static const int N = 2048;
    static constexpr float MIN_FREQ = 20.0f;
    static constexpr float MAX_FREQ = 20000.0f;
    static constexpr int SAMPLE_RATE = 44100;
}; 