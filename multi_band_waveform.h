#ifndef MULTI_BAND_WAVEFORM_H
#define MULTI_BAND_WAVEFORM_H

#include "visualizer_base.h"
#include <vector>

class MultiBandWaveform : public Visualizer
{
public:
    MultiBandWaveform();
    ~MultiBandWaveform() override;

    // Add method to set multiple audio sources
    void setAudioSources(const std::vector<std::vector<float>>& sources);

    // Implement the base class methods
    void renderFrame(const std::vector<float> &audioData,
                     double *fftInputBuffer,
                     fftw_complex *fftOutputBuffer,
                     fftw_plan &fftPlan,
                     float timeSeconds) override;

    void renderLiveFrame(const std::vector<float> &audioData,
                         double *fftInputBuffer,
                         fftw_complex *fftOutputBuffer,
                         fftw_plan &fftPlan,
                         size_t currentPosition) override;

private:
    // Helper method to render a single band
    void renderBand(const std::vector<float> &bandData, float yOffset, float height, float xOffset, float width, const float *color);

    // Helper method to filter audio data into frequency bands
    std::vector<float> filterBand(fftw_complex *fftOutput, int startBin, int endBin);

    // Helper method to calculate grid dimensions
    void calculateGridDimensions(int numSources, int& rows, int& cols) const;

    // Helper method to process audio data for FFT
    void processAudioForFFT(const std::vector<float>& audioData, size_t position, double* fftInputBuffer);

    const int N = 1024;                       // FFT size
    static constexpr int LOW_CUTOFF = 250;    // 20-250 Hz
    static constexpr int MID_CUTOFF = 2000;   // 250-2000 Hz
    static constexpr int HIGH_CUTOFF = 20000; // 2000-20000 Hz

    // Colors for each band
    static constexpr float LOW_COLOR[3] = {1.0f, 0.0f, 0.0f};  // Red
    static constexpr float MID_COLOR[3] = {0.0f, 1.0f, 0.0f};  // Green
    static constexpr float HIGH_COLOR[3] = {0.0f, 0.0f, 1.0f}; // Blue

    // Store multiple audio sources
    std::vector<std::vector<float>> audioSources;
};

#endif // MULTI_BAND_WAVEFORM_H