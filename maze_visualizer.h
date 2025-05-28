#pragma once
#include "visualizer_base.h"
#include <vector>
#include <deque>

class MazeVisualizer : public Visualizer
{
public:
    MazeVisualizer();
    ~MazeVisualizer();

    void initialize(int width, int height) override;

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
    static constexpr int MAZE_SIZE = 32;           // Larger maze for more complexity
    static constexpr float CELL_SIZE = 0.8f;       // Larger cells for better corridors
    static constexpr float WALL_HEIGHT = 1.5f;     // Taller walls for better maze feel
    static constexpr float MOVE_SPEED = 1.5f;      // Slower movement to appreciate the maze
    static constexpr float TUNNEL_WIDTH = 0.3f;    // Width of the tunnel path
    static constexpr float PULSE_INTENSITY = 0.3f; // Reduced pulse for more stable walls

    // Green vector colors
    static constexpr float WALL_COLOR[3] = {0.0f, 1.0f, 0.0f};    // Bright green
    static constexpr float FLOOR_COLOR[3] = {0.0f, 0.3f, 0.0f};   // Dark green
    static constexpr float CEILING_COLOR[3] = {0.0f, 0.5f, 0.0f}; // Medium green
    static constexpr float GLOW_COLOR[3] = {0.2f, 1.0f, 0.2f};    // Bright green glow

    struct MazeCell
    {
        bool hasWall;
        float height;
        float glowIntensity;
    };

    struct TunnelSegment
    {
        float x, z;
        float rotation;
        float width;
    };

    float audioAmplitude;
    float mazePosition;
    float cameraRotation;
    std::vector<std::vector<MazeCell>> maze;
    std::deque<TunnelSegment> tunnelPath;

    void generateMaze();
    void updateMaze(float deltaTime);
    void updateTunnel(float deltaTime);
    void renderMazeWalls();
    void renderFloorAndCeiling();
    void renderTunnelEffects();
    void setupPerspectiveView();
    float calculateAudioAmplitude(const std::vector<float> &audioData, size_t position);
    void createTunnelSegment(float x, float z, float rotation);
};