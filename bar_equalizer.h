#ifndef BAR_EQUALIZER_H
#define BAR_EQUALIZER_H

#include "visualizer_base.h"
#include <vector>

class BarEqualizer : public Visualizer
{
public:
    explicit BarEqualizer(int numBars = 32);
    ~BarEqualizer() override;

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

#endif // BAR_EQUALIZER_H