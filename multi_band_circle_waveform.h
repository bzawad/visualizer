#ifndef MULTI_BAND_CIRCLE_WAVEFORM_H
#define MULTI_BAND_CIRCLE_WAVEFORM_H

#include "visualizer_base.h"
#include <vector>

class MultiBandCircleWaveform : public Visualizer
{
public:
    MultiBandCircleWaveform();
    ~MultiBandCircleWaveform() override;

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
    // Helper method to render a single circular band
    void renderCircularBand(const std::vector<float> &bandData, float radius, float thickness, const float *color);

    // Helper method to filter audio data into frequency bands
    std::vector<float> filterBand(fftw_complex *fftOutput, int startBin, int endBin, float bandScaling);

    const int N = 1024;                       // FFT size
    static constexpr int LOW_CUTOFF = 250;    // 20-250 Hz
    static constexpr int MID_CUTOFF = 2000;   // 250-2000 Hz
    static constexpr int HIGH_CUTOFF = 20000; // 2000-20000 Hz

    // Colors for each band
    static constexpr float LOW_COLOR[3] = {1.0f, 0.0f, 0.0f};  // Red (inner)
    static constexpr float MID_COLOR[3] = {0.0f, 1.0f, 0.0f};  // Green (middle)
    static constexpr float HIGH_COLOR[3] = {0.0f, 0.0f, 1.0f}; // Blue (outer)

    // Radii for each band
    static constexpr float LOW_RADIUS = 0.2f;
    static constexpr float MID_RADIUS = 0.5f;
    static constexpr float HIGH_RADIUS = 0.8f;
    static constexpr float THICKNESS = 0.15f;
};

#endif // MULTI_BAND_CIRCLE_WAVEFORM_H