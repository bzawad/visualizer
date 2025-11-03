#include "mini_racer_visualizer.h"
#include <GL/glew.h>
#include <cmath>
#include <algorithm>

#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

MiniRacerVisualizer::MiniRacerVisualizer() : audioAmplitude(0.0f), roadPosition(0.0f)
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
        // Distribute buildings from far to near
        float z = -5.0f + (8.0f * i / NUM_BUILDINGS); // From -5 to 3

        // Calculate width at this z position (linear interpolation)
        float t = (z - 3.0f) / (-5.0f - 3.0f);
        float roadWidthAtZ = ROAD_WIDTH * 2.5f * (1.0f - t) + ROAD_WIDTH * 0.9f * t;

        // Add some random variation to building height
        float heightVariation = 0.8f + (rand() % 100) / 100.0f * 0.4f;

        // Position buildings *outside* the road edges (add offset)
        float buildingOffset = 0.2f;

        // Left building - position outside road edge
        leftBuildings.push_back({BUILDING_HEIGHT * heightVariation,
                                 -(roadWidthAtZ + buildingOffset), // Offset from road edge
                                 z});

        // Right building - position outside road edge
        rightBuildings.push_back({BUILDING_HEIGHT * heightVariation,
                                  (roadWidthAtZ + buildingOffset), // Offset from road edge
                                  z});
    }
}

MiniRacerVisualizer::~MiniRacerVisualizer() {}

void MiniRacerVisualizer::initialize(int width, int height)
{
    // Force resolution to 128x43 (ignore passed parameters)
    (void)width;
    (void)height;
    Visualizer::initialize(128, 43);
}

void MiniRacerVisualizer::setupPerspectiveView()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float aspect = static_cast<float>(screenWidth) / screenHeight;

#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
    gluPerspective(80.0f, aspect, 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Position camera lower and closer to the road for a driver's perspective
    gluLookAt(0.0f, 0.6f, 1.8f,  // Eye
              0.0f, 0.1f, -5.0f, // Target
              0.0f, 1.0f, 0.0f); // Up
#ifdef __APPLE__
#pragma clang diagnostic pop
#endif
}

void MiniRacerVisualizer::updateRoad(float deltaTime)
{
    for (float &z : roadLines)
    {
        z -= deltaTime * MOVE_SPEED;
        if (z < -1.0f)
            z = 1.0f;
    }
}

void MiniRacerVisualizer::updateBuildings(float deltaTime)
{
    // We'll reuse the same calculations but remove redundant computations
    const float zRange = (-5.0f - 3.0f); // -8.0f
    float move = deltaTime * MOVE_SPEED;

    // Left buildings
    for (auto &building : leftBuildings)
    {
        building.zPos += move;
        if (building.zPos > 3.0f)
        {
            // Wrap to far point
            building.zPos = -5.0f;
        }

        // Interpolate road width
        float t = (building.zPos - 3.0f) / zRange;
        float roadWidthAtZ = ROAD_WIDTH * 2.5f * (1.0f - t) + ROAD_WIDTH * 0.9f * t;

        // Audio-based sine wave
        float wave = std::sin(building.zPos * SINE_FREQ + roadPosition) * audioAmplitude;
        building.height = BUILDING_HEIGHT * (1.0f + wave * 1.5f);

        // Position the building
        building.xPos = -(roadWidthAtZ + 0.2f);
    }

    // Right buildings
    for (auto &building : rightBuildings)
    {
        building.zPos += move;
        if (building.zPos > 3.0f)
        {
            // Wrap to far point
            building.zPos = -5.0f;
        }

        float t = (building.zPos - 3.0f) / zRange;
        float roadWidthAtZ = ROAD_WIDTH * 2.5f * (1.0f - t) + ROAD_WIDTH * 0.9f * t;

        float wave = std::sin(building.zPos * SINE_FREQ + roadPosition) * audioAmplitude;
        building.height = BUILDING_HEIGHT * (1.0f + wave * 1.5f);

        building.xPos = (roadWidthAtZ + 0.2f);
    }
}

void MiniRacerVisualizer::renderRoad()
{
    glLineWidth(2.0f);

    float nearZ = 3.0f;
    float farZ = -5.0f;
    float nearWidth = ROAD_WIDTH * 2.5f;
    float farWidth = ROAD_WIDTH * 0.9f;

    // Merge road edges + horizontal lines into one GL_LINES call
    glBegin(GL_LINES);

    // Road edges
    glColor3fv(ROAD_COLOR);
    glVertex3f(-nearWidth, 0.0f, nearZ);
    glVertex3f(-farWidth, 0.0f, farZ);

    glVertex3f(nearWidth, 0.0f, nearZ);
    glVertex3f(farWidth, 0.0f, farZ);

    // Horizontal grid lines
    glColor3fv(GRID_COLOR);
    for (float z : roadLines)
    {
        float mappedZ = nearZ + (z + 1.0f) * (farZ - nearZ) * 0.5f; // same as /2.0f
        float t = (mappedZ - nearZ) / (farZ - nearZ);
        float width = nearWidth * (1.0f - t) + farWidth * t;

        glVertex3f(-width, 0.0f, mappedZ);
        glVertex3f(width, 0.0f, mappedZ);
    }

    glEnd();
}

void MiniRacerVisualizer::renderBuildings()
{
    glLineWidth(2.0f);

    // Combine left & right buildings into a single draw call
    glBegin(GL_LINES);
    glColor3fv(BUILDING_COLOR);

    for (const auto &building : leftBuildings)
    {
        glVertex3f(building.xPos, 0.0f, building.zPos);
        glVertex3f(building.xPos, building.height, building.zPos);
    }
    for (const auto &building : rightBuildings)
    {
        glVertex3f(building.xPos, 0.0f, building.zPos);
        glVertex3f(building.xPos, building.height, building.zPos);
    }

    glEnd();
}

float MiniRacerVisualizer::calculateAudioAmplitude(const std::vector<float> &audioData, size_t position)
{
    const size_t windowSize = 1024;
    // Clamp the end so we don't read beyond the audioData
    size_t end = std::min(position + windowSize, audioData.size());

    float sum = 0.0f;
    for (size_t i = position; i < end; i++)
    {
        sum += std::fabs(audioData[i]);
    }

    float average = 0.0f;
    size_t count = end - position;
    if (count > 0)
        average = sum / static_cast<float>(count);

    // Scale for dramatic effect but max out at 1.0
    return std::min(1.0f, average * 4.0f);
}

void MiniRacerVisualizer::renderSun()
{
    // Enable blending for glow effect
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float horizonY = 0.0f; // The Y coordinate of the horizon

    // Draw sun at horizon
    glPushMatrix();
    glTranslatef(0.0f, horizonY, SUN_Z_POS);

    // Half-circle (upper half)
    glBegin(GL_TRIANGLE_FAN);
    // Center - inner color
    glColor3fv(SUN_INNER_COLOR);
    glVertex3f(0.0f, 0.0f, 0.0f);

    // Outer - outer color
    glColor3fv(SUN_OUTER_COLOR);
    for (int i = 0; i <= SUN_SEGMENTS; i++)
    {
        float angle = M_PI * i / SUN_SEGMENTS;
        float x = SUN_RADIUS * std::cos(angle) * 1.25f;
        float y = SUN_RADIUS * std::sin(angle) * 1.25f;
        if (y >= 0.0f)
        {
            glVertex3f(x, y, 0.0f);
        }
    }
    glEnd();

    // Rays - in green
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glColor4f(0.0f, 1.0f, 0.0f, 0.5f);
    for (int i = 0; i < 12; i++)
    {
        float angle = M_PI * i / 11;
        if (std::sin(angle) >= 0.0f)
        {
            float x1 = SUN_RADIUS * std::cos(angle) * 1.25f;
            float y1 = SUN_RADIUS * std::sin(angle) * 1.25f;
            float x2 = (SUN_RADIUS * 1.3f) * std::cos(angle) * 1.25f;
            float y2 = (SUN_RADIUS * 1.3f) * std::sin(angle) * 1.25f;

            glVertex3f(x1, y1, 0.0f);
            glVertex3f(x2, y2, 0.0f);
        }
    }
    glEnd();

    glPopMatrix();
}

void MiniRacerVisualizer::renderFrame(const std::vector<float> &audioData,
                                      double * /* fftInputBuffer */,
                                      fftw_complex * /* fftOutputBuffer */,
                                      fftw_plan & /* fftPlan */,
                                      float timeSeconds)
{
    size_t sampleIndex = static_cast<size_t>(timeSeconds * 44100);
    audioAmplitude = calculateAudioAmplitude(audioData, sampleIndex);

    // Move the road
    roadPosition = std::fmod(roadPosition + ROAD_SPEED, 1.0f);

    setupPerspectiveView();

    // Enable line smoothing
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Furthest away first
    renderSun();

    // Update and render
    updateRoad(1.0f / 60.0f);
    updateBuildings(1.0f / 60.0f);
    renderRoad();
    renderBuildings();

    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
}

void MiniRacerVisualizer::renderLiveFrame(const std::vector<float> &audioData,
                                          double * /* fftInputBuffer */,
                                          fftw_complex * /* fftOutputBuffer */,
                                          fftw_plan & /* fftPlan */,
                                          size_t currentPosition)
{
    audioAmplitude = calculateAudioAmplitude(audioData, currentPosition);
    roadPosition = std::fmod(roadPosition + ROAD_SPEED, 1.0f);

    setupPerspectiveView();

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderSun();

    updateRoad(1.0f / 60.0f);
    updateBuildings(1.0f / 60.0f);
    renderRoad();
    renderBuildings();

    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
}
