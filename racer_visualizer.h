#pragma once
#include "visualizer_base.h"
#include <vector>
#include <deque>

class RacerVisualizer : public Visualizer
{
public:
    RacerVisualizer();
    ~RacerVisualizer();

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
    static constexpr int NUM_ROAD_LINES = 30;      // More lines for smoother road
    static constexpr int NUM_BUILDINGS = 40;       // More buildings for better density
    static constexpr float ROAD_WIDTH = 1.0f;      // Base road width
    static constexpr float BUILDING_HEIGHT = 1.0f; // Slightly taller buildings
    static constexpr float MOVE_SPEED = 2.5f;      // Faster movement for better effect
    static constexpr float SINE_FREQ = 3.0f;       // Higher frequency for wave effect

    // Colors
    static constexpr float ROAD_COLOR[3] = {0.0f, 0.6f, 0.8f};     // Cyan
    static constexpr float BUILDING_COLOR[3] = {0.8f, 0.0f, 0.8f}; // Magenta
    static constexpr float GRID_COLOR[3] = {0.4f, 0.0f, 0.4f};     // Dark purple

    // Sun parameters
    static constexpr float SUN_RADIUS = 0.675f;                       // Match road width at horizon
    static constexpr float SUN_Y_POS = 0.0f;                        // Position at horizon
    static constexpr float SUN_Z_POS = -5.0f;                       // Match far end of road
    static constexpr int SUN_SEGMENTS = 40;                         // More segments for smoother circle
    static constexpr float SUN_INNER_COLOR[3] = {1.0f, 0.6f, 0.0f}; // Brighter orange
    static constexpr float SUN_OUTER_COLOR[3] = {0.9f, 0.1f, 0.9f}; // Brighter magenta

    struct Building
    {
        float height;
        float xPos;
        float zPos;
    };

    float audioAmplitude;
    float roadPosition; // Position of the road (0.0 to 1.0)
    std::deque<Building> leftBuildings;
    std::deque<Building> rightBuildings;
    std::vector<float> roadLines;

    void updateRoad(float deltaTime);
    void updateBuildings(float deltaTime);
    void renderRoad();
    void renderBuildings();
    void renderSun();
    float calculateAudioAmplitude(const std::vector<float> &audioData, size_t position);
    void setupPerspectiveView();

    static constexpr float ROAD_SPEED = 0.02f;
};