#ifndef MULTI_BAND_WAVEFORM_H
#define MULTI_BAND_WAVEFORM_H

#include "visualizer_base.h"

class MultiBandWaveform : public Visualizer
{
public:
    MultiBandWaveform();
    ~MultiBandWaveform() override;

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
    void renderBand(const std::vector<float> &bandData, float yOffset, float height, const float *color);

    // Helper method to filter audio data into frequency bands
    std::vector<float> filterBand(fftw_complex *fftOutput, int startBin, int endBin);

    const int N = 1024;            // FFT size
    const int LOW_CUTOFF = 200;    // Hz
    const int MID_CUTOFF = 2000;   // Hz
    const int HIGH_CUTOFF = 20000; // Hz

    // Colors for each band
    const float LOW_COLOR[3] = {0.0f, 0.0f, 1.0f};  // Blue
    const float MID_COLOR[3] = {0.0f, 1.0f, 0.0f};  // Green
    const float HIGH_COLOR[3] = {1.0f, 0.0f, 0.0f}; // Red
};

#endif // MULTI_BAND_WAVEFORM_H