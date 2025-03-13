#ifndef GRID_VISUALIZER_H
#define GRID_VISUALIZER_H

#include "visualizer_base.h"
#include <vector>

class GridVisualizer : public Visualizer
{
public:
    GridVisualizer();
    ~GridVisualizer() override;

    // Add method to set multiple audio sources
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
    // Helper method to process audio data for FFT
    void processAudioForFFT(const std::vector<float>& audioData, size_t position, double* fftInputBuffer);

    // Helper method to render the grid
    void renderGrid(const std::vector<float>& magnitudes);

    static constexpr int GRID_SIZE = 32;      // 32x32 grid
    static constexpr int N = 1024;            // FFT size
    static constexpr float MIN_FREQ = 20.0f;  // Minimum frequency (Hz)
    static constexpr float MAX_FREQ = 20000.0f; // Maximum frequency (Hz)

    // Store multiple audio sources
    std::vector<std::vector<float>> audioSources;
};

#endif // GRID_VISUALIZER_H 