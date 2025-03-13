#pragma once

#include <vector>
#include <GL/glew.h>
#include <fftw3.h>

class Visualizer {
public:
    // Constructor and destructor
    Visualizer() {}
    virtual ~Visualizer() = default;

    // Initialize the visualizer with common settings
    virtual void initialize(int width, int height) {
        screenWidth = width;
        screenHeight = height;
    }
    
    // Multi-source methods
    virtual void renderFrame(const std::vector<std::vector<float>>& audioSources,
                           double* in,
                           fftw_complex* out,
                           fftw_plan& plan,
                           float timeSeconds) {
        // Default implementation for backward compatibility
        renderFrame(audioSources.empty() ? std::vector<float>() : audioSources[0], in, out, plan, timeSeconds);
    }

    virtual void renderLiveFrame(const std::vector<std::vector<float>>& audioSources,
                               double* in,
                               fftw_complex* out,
                               fftw_plan& plan,
                               size_t currentPosition) {
        // Default implementation for backward compatibility
        renderLiveFrame(audioSources.empty() ? std::vector<float>() : audioSources[0], in, out, plan, currentPosition);
    }

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
}; 