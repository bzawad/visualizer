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
    static constexpr int NUM_ROAD_LINES = 20;      // Number of horizontal lines on road
    static constexpr int NUM_BUILDINGS = 30;       // Number of buildings on each side
    static constexpr float ROAD_WIDTH = 1.2f;      // Width of the road
    static constexpr float BUILDING_HEIGHT = 0.8f; // Base height of buildings
    static constexpr float MOVE_SPEED = 2.0f;      // Speed of forward motion
    static constexpr float SINE_FREQ = 2.0f;       // Frequency of building wave

    // Colors
    static constexpr float ROAD_COLOR[3] = {0.0f, 0.6f, 0.8f};     // Cyan
    static constexpr float BUILDING_COLOR[3] = {0.8f, 0.0f, 0.8f}; // Magenta
    static constexpr float GRID_COLOR[3] = {0.4f, 0.0f, 0.4f};     // Dark purple

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
    float calculateAudioAmplitude(const std::vector<float> &audioData, size_t position);
    void setupPerspectiveView();

    static constexpr float ROAD_SPEED = 0.02f;
};