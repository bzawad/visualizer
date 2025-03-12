#ifndef ASCII_BAR_EQUALIZER_H
#define ASCII_BAR_EQUALIZER_H

#include "visualizer_base.h"
#include <random>

class AsciiBarEqualizer : public Visualizer
{
public:
    AsciiBarEqualizer(int numBars = 16);
    ~AsciiBarEqualizer() override;

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
    // Helper method for actual rendering
    void renderBars(fftw_complex *fftOutputBuffer);

    // Helper to render a single ASCII bar
    void renderAsciiBar(float xLeft, float xRight, float height);

    int numBars;
    const int N = 1024; // FFT size

    // Random number generation for ASCII characters
    std::mt19937 rng;
    std::uniform_int_distribution<int> dist;
};

#endif // ASCII_BAR_EQUALIZER_H