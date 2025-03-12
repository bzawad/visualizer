#ifndef TERRAIN_VISUALIZER_3D_H
#define TERRAIN_VISUALIZER_3D_H

#include "visualizer_base.h"
#include <vector>
#include <array>

class TerrainVisualizer3D : public Visualizer
{
public:
    TerrainVisualizer3D();
    ~TerrainVisualizer3D() override;

    // Initialize with custom settings
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
    // Number of frequency bands
    static constexpr int NUM_BANDS = 3;
    
    // Frequency range boundaries for the bands (Hz) - same as multi_band_waveform
    static constexpr int LOW_CUTOFF = 250;    // 20-250 Hz
    static constexpr int MID_CUTOFF = 2000;   // 250-2000 Hz
    static constexpr int HIGH_CUTOFF = 20000; // 2000-20000 Hz
    
    // Colors for each band - all green for retro look
    static constexpr float BAND_COLOR[3] = {0.0f, 1.0f, 0.2f};  // Bright green
    static constexpr float GRID_COLOR[3] = {0.0f, 0.3f, 0.1f};  // Dim green
    
    // Rendering parameters
    static constexpr int POINTS_PER_BAND = 200;   // Resolution of each band
    static constexpr float LINE_WIDTH = 5.0f;     // Thickness of lines
    static constexpr float TERRAIN_WIDTH = 2.0f;  // Width of the visualization
    static constexpr float TERRAIN_HEIGHT = 1.0f; // Max height of the visualization
    static constexpr float BAND_SPACING = 0.6f;   // Spacing between bands
    
    // FFT analysis outputs for the 3 bands
    std::array<std::vector<float>, NUM_BANDS> bandData;
    
    // Helper methods
    void setupPerspectiveView();
    void analyzeBands(fftw_complex *fftOutput);
    void renderTerrain();
    void updateFFT(const std::vector<float> &audioData, double *fftInputBuffer, fftw_plan &fftPlan, size_t position);
};

#endif // TERRAIN_VISUALIZER_3D_H 