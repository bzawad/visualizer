#ifndef SPECTROGRAM_H
#define SPECTROGRAM_H

#include "visualizer_base.h"
#include <vector>

class Spectrogram : public Visualizer
{
public:
    Spectrogram();
    ~Spectrogram() override;

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
    void renderSpectrum(const fftw_complex *fftData);
    const int N = 1024;        // FFT size
    std::vector<float> window; // Hanning window for better frequency resolution
};

#endif // SPECTROGRAM_H