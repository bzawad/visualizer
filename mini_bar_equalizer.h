#ifndef MINI_BAR_EQUALIZER_H
#define MINI_BAR_EQUALIZER_H

#include "visualizer_base.h"
#include <vector>

class MiniBarEqualizer : public Visualizer
{
public:
    explicit MiniBarEqualizer(int numBars = 32);
    ~MiniBarEqualizer() override;

    void initialize(int width, int height) override;

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
    // Helper method for actual rendering (used by both render methods)
    void renderBars(fftw_complex *fftOutputBuffer);

    const int numBars;
    const int N = 1024; // FFT size

    // Peak tracking
    std::vector<float> peakHeights;
    std::vector<float> peakDecay;
    static constexpr float PEAK_DECAY_RATE = 0.005f; // How fast peaks fall
    static constexpr float PEAK_HEIGHT = 0.9f;       // Peak line at 90% of bar height
};

#endif // MINI_BAR_EQUALIZER_H
