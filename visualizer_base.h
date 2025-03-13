#pragma once

#include <vector>
#include <GL/glew.h>
#include <fftw3.h>

class Visualizer {
public:
    // Constructor and destructor
    Visualizer();
    virtual ~Visualizer();

    // Initialize the visualizer with common settings
    virtual void initialize(int width, int height);
    
    // Multi-source methods
    virtual void renderFrame(const std::vector<std::vector<float>>& audioSources,
                           double* in,
                           fftw_complex* out,
                           fftw_plan& plan,
                           float timeSeconds);

    virtual void renderLiveFrame(const std::vector<std::vector<float>>& audioSources,
                               double* in,
                               fftw_complex* out,
                               fftw_plan& plan,
                               size_t currentPosition);

    // Legacy single-source methods
    virtual void renderFrame(const std::vector<float>& audioData,
                           double* in,
                           fftw_complex* out,
                           fftw_plan& plan,
                           float timeSeconds) = 0;
                           
    virtual void renderLiveFrame(const std::vector<float>& audioData,
                               double* in,
                               fftw_complex* out,
                               fftw_plan& plan,
                               size_t currentPosition) = 0;

protected:
    int screenWidth = 800;
    int screenHeight = 600;
    static const int N = 2048;  // FFT size
}; 