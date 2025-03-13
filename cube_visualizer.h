#pragma once

#include "visualizer_base.h"
#include <vector>
#include <array>

class CubeVisualizer : public Visualizer {
public:
    CubeVisualizer();
    ~CubeVisualizer() override = default;

    void initialize(int width, int height) override;
    
    void renderFrame(const std::vector<float>& audioData,
                    double* in,
                    fftw_complex* out,
                    fftw_plan& plan,
                    float timeSeconds) override;
                    
    void renderLiveFrame(const std::vector<float>& audioData,
                        double* in,
                        fftw_complex* out,
                        fftw_plan& plan,
                        size_t currentPosition) override;

private:
    void render(float time, const std::vector<float>& magnitudes);
    void drawCube(float rotationAngle, float scale);
    std::vector<float> calculateMagnitudes(fftw_complex* out);
    
    // Cube vertices (8 corners)
    static constexpr std::array<float, 24> vertices = {
        -0.5f, -0.5f, -0.5f,  // 0: back-bottom-left
         0.5f, -0.5f, -0.5f,  // 1: back-bottom-right
         0.5f,  0.5f, -0.5f,  // 2: back-top-right
        -0.5f,  0.5f, -0.5f,  // 3: back-top-left
        -0.5f, -0.5f,  0.5f,  // 4: front-bottom-left
         0.5f, -0.5f,  0.5f,  // 5: front-bottom-right
         0.5f,  0.5f,  0.5f,  // 6: front-top-right
        -0.5f,  0.5f,  0.5f   // 7: front-top-left
    };
    
    // Cube edges (12 lines connecting vertices)
    static constexpr std::array<int, 24> edges = {
        0, 1,  1, 2,  2, 3,  3, 0,  // Back face
        4, 5,  5, 6,  6, 7,  7, 4,  // Front face
        0, 4,  1, 5,  2, 6,  3, 7   // Connecting edges
    };
    
    // Rendering parameters
    static constexpr float LINE_WIDTH = 5.0f;
    static constexpr float BASE_ROTATION_SPEED = 0.5f;
    static constexpr float MAX_ROTATION_SPEED = 8.0f;
    static constexpr float BASE_SCALE = 0.6f;
    static constexpr float MAX_SCALE = 1.2f;
    static constexpr float BOUNCE_FACTOR = 0.6f;
    
    // Audio analysis parameters
    static constexpr int PITCH_START_BIN = 10;    // ~200 Hz
    static constexpr int PITCH_END_BIN = 100;     // ~2000 Hz
    static constexpr int AMPLITUDE_START_BIN = 0;  // 0 Hz
    static constexpr int AMPLITUDE_END_BIN = 30;   // ~600 Hz
    
    float aspectRatio;
}; 