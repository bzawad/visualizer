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
    static constexpr int NUM_BANDS = 5;
    
    // Frequency range boundaries for the bands (Hz)
    static constexpr int LOWEST_CUTOFF = 150;    // 20-150 Hz
    static constexpr int LOW_MID_CUTOFF = 500;   // 150-500 Hz
    static constexpr int MID_CUTOFF = 2000;      // 500-2000 Hz
    static constexpr int MID_HIGH_CUTOFF = 8000; // 2000-8000 Hz
    static constexpr int HIGH_CUTOFF = 20000;    // 8000-20000 Hz
    
    // Colors for each band - all green for retro look
    static constexpr float BAND_COLOR[3] = {0.0f, 1.0f, 0.2f};  // Bright green
    static constexpr float GRID_COLOR[3] = {0.0f, 0.3f, 0.1f};  // Dim green
    
    // Rendering parameters
    static constexpr int POINTS_PER_BAND = 200;   // Resolution of each band
    static constexpr float LINE_WIDTH = 5.0f;     // Thickness of lines
    static constexpr float TERRAIN_WIDTH = 12.0f; // Width of the visualization
    static constexpr float TERRAIN_HEIGHT = 2.5f; // Max height of the visualization (increased from 1.5)
    static constexpr float BAND_SPACING = 0.6f;   // Spacing between bands
    
    // FFT analysis outputs for the bands
    std::array<std::vector<float>, NUM_BANDS> bandData;
    
    // Helper methods
    void setupPerspectiveView();
    void analyzeBands(fftw_complex *fftOutput);
    void renderTerrain();
    void updateFFT(const std::vector<float> &audioData, double *fftInputBuffer, fftw_plan &fftPlan, size_t position);
};

#endif // TERRAIN_VISUALIZER_3D_H 