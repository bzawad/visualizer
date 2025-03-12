#ifndef BAR_EQUALIZER_H
#define BAR_EQUALIZER_H

#include "visualizer_base.h"

class BarEqualizer : public Visualizer {
public:
    BarEqualizer(int numBars = 16);
    ~BarEqualizer() override;

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
    // Helper method for actual rendering (used by both render methods)
    void renderBars(fftw_complex* fftOutputBuffer);

    int numBars;
    const int N = 1024; // FFT size
};

#endif // BAR_EQUALIZER_H 