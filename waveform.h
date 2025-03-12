#ifndef WAVEFORM_H
#define WAVEFORM_H

#include "visualizer_base.h"

class Waveform : public Visualizer {
public:
    Waveform();
    ~Waveform() override;

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
    const int N = 1024; // Number of samples to display
};

#endif // WAVEFORM_H 