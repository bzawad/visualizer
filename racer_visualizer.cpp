#include "racer_visualizer.h"
#include <GL/glew.h>
#include <cmath>
#include <algorithm>

#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

RacerVisualizer::RacerVisualizer() : audioAmplitude(0.0f), roadPosition(0.0f)
{
    // Initialize road lines
    roadLines.resize(NUM_ROAD_LINES);
    float spacing = 2.0f / NUM_ROAD_LINES;
    for (int i = 0; i < NUM_ROAD_LINES; i++)
    {
        roadLines[i] = -1.0f + i * spacing;
    }

    // Initialize buildings
    for (int i = 0; i < NUM_BUILDINGS; i++)
    {
        float z = -1.0f + (2.0f * i / NUM_BUILDINGS);

        // Left building
        leftBuildings.push_back({BUILDING_HEIGHT, -ROAD_WIDTH, z});

        // Right building
        rightBuildings.push_back({BUILDING_HEIGHT, ROAD_WIDTH, z});
    }
}

RacerVisualizer::~RacerVisualizer() {}

void RacerVisualizer::initialize(int width, int height)
{
    Visualizer::initialize(width, height);
}

void RacerVisualizer::setupPerspectiveView()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float aspect = static_cast<float>(screenWidth) / screenHeight;
    // Suppress macOS deprecation warning with pragma
#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
    gluPerspective(60.0f, aspect, 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Position camera above and behind to look down the road
    gluLookAt(0.0f, 1.0f, 2.0f,  // Eye position
              0.0f, 0.0f, -1.0f, // Look target
              0.0f, 1.0f, 0.0f); // Up vector
#ifdef __APPLE__
#pragma clang diagnostic pop
#endif
}

void RacerVisualizer::updateRoad(float deltaTime)
{
    // Move road lines toward viewer
    for (float &z : roadLines)
    {
        z += deltaTime * MOVE_SPEED;
        if (z > 1.0f)
            z = -1.0f;
    }
}

void RacerVisualizer::updateBuildings(float deltaTime)
{
    // Update building positions and heights
    for (auto &building : leftBuildings)
    {
        building.zPos += deltaTime * MOVE_SPEED;
        if (building.zPos > 1.0f)
        {
            building.zPos = -1.0f;
        }

        // Calculate sine wave height with audio amplitude
        float wave = std::sin(building.zPos * SINE_FREQ + roadPosition) * audioAmplitude;
        building.height = BUILDING_HEIGHT * (1.0f + wave);
    }

    for (auto &building : rightBuildings)
    {
        building.zPos += deltaTime * MOVE_SPEED;
        if (building.zPos > 1.0f)
        {
            building.zPos = -1.0f;
        }

        // Calculate sine wave height with audio amplitude
        float wave = std::sin(building.zPos * SINE_FREQ + roadPosition) * audioAmplitude;
        building.height = BUILDING_HEIGHT * (1.0f + wave);
    }
}

void RacerVisualizer::renderRoad()
{
    glLineWidth(2.0f);

    // Draw road edges
    glColor3fv(ROAD_COLOR);
    glBegin(GL_LINES);
    glVertex3f(-ROAD_WIDTH, 0.0f, -1.0f);
    glVertex3f(-ROAD_WIDTH, 0.0f, 1.0f);
    glVertex3f(ROAD_WIDTH, 0.0f, -1.0f);
    glVertex3f(ROAD_WIDTH, 0.0f, 1.0f);
    glEnd();

    // Draw horizontal lines
    glColor3fv(GRID_COLOR);
    glBegin(GL_LINES);
    for (float z : roadLines)
    {
        glVertex3f(-ROAD_WIDTH, 0.0f, z);
        glVertex3f(ROAD_WIDTH, 0.0f, z);
    }
    glEnd();
}

void RacerVisualizer::renderBuildings()
{
    glLineWidth(2.0f);
    glColor3fv(BUILDING_COLOR);

    // Draw left buildings
    glBegin(GL_LINES);
    for (const auto &building : leftBuildings)
    {
        glVertex3f(building.xPos, 0.0f, building.zPos);
        glVertex3f(building.xPos, building.height, building.zPos);
    }
    glEnd();

    // Draw right buildings
    glBegin(GL_LINES);
    for (const auto &building : rightBuildings)
    {
        glVertex3f(building.xPos, 0.0f, building.zPos);
        glVertex3f(building.xPos, building.height, building.zPos);
    }
    glEnd();
}

float RacerVisualizer::calculateAudioAmplitude(const std::vector<float> &audioData, size_t position)
{
    float sum = 0.0f;
    const size_t windowSize = 1024;

    for (size_t i = 0; i < windowSize && (position + i) < audioData.size(); i++)
    {
        sum += std::abs(audioData[position + i]);
    }

    return std::min(1.0f, sum / windowSize * 4.0f); // Scale for more dramatic effect
}

void RacerVisualizer::renderFrame(const std::vector<float> &audioData,
                                  double * /* fftInputBuffer */,
                                  fftw_complex * /* fftOutputBuffer */,
                                  fftw_plan & /* fftPlan */,
                                  float timeSeconds)
{
    // Calculate audio amplitude
    size_t sampleIndex = static_cast<size_t>(timeSeconds * 44100);
    audioAmplitude = calculateAudioAmplitude(audioData, sampleIndex);

    // Update road position (continuous movement)
    roadPosition = std::fmod(roadPosition + ROAD_SPEED, 1.0f);

    // Set up 3D view
    setupPerspectiveView();

    // Enable line smoothing
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update and render components
    updateRoad(1.0f / 60.0f);
    updateBuildings(1.0f / 60.0f);
    renderRoad();
    renderBuildings();

    // Disable features
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
}

void RacerVisualizer::renderLiveFrame(const std::vector<float> &audioData,
                                      double * /* fftInputBuffer */,
                                      fftw_complex * /* fftOutputBuffer */,
                                      fftw_plan & /* fftPlan */,
                                      size_t currentPosition)
{
    // Calculate audio amplitude
    audioAmplitude = calculateAudioAmplitude(audioData, currentPosition);

    // Update road position (continuous movement)
    roadPosition = std::fmod(roadPosition + ROAD_SPEED, 1.0f);

    // Set up 3D view
    setupPerspectiveView();

    // Enable line smoothing
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update and render components
    updateRoad(1.0f / 60.0f);
    updateBuildings(1.0f / 60.0f);
    renderRoad();
    renderBuildings();

    // Disable features
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
}