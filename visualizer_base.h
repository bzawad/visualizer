#ifndef VISUALIZER_BASE_H
#define VISUALIZER_BASE_H

#include <vector>
#include <GL/glew.h>
#include <fftw3.h>

class Visualizer {
public:
    // Constructor and destructor
    Visualizer() {}
    virtual ~Visualizer() {}

    // Initialize the visualizer with common settings
    virtual void initialize(int width, int height) {
        screenWidth = width;
        screenHeight = height;
    }

    // Render frame for offline (recording) mode
    virtual void renderFrame(const std::vector<float>& audioData, 
                           double* fftInputBuffer, 
                           fftw_complex* fftOutputBuffer,
                           fftw_plan& fftPlan, 
                           float timeSeconds) = 0;

    // Render frame for live playback mode
    virtual void renderLiveFrame(const std::vector<float>& audioData, 
                               double* fftInputBuffer, 
                               fftw_complex* fftOutputBuffer,
                               fftw_plan& fftPlan, 
                               size_t currentPosition) = 0;

protected:
    int screenWidth = 800;
    int screenHeight = 600;
};

#endif // VISUALIZER_BASE_H 