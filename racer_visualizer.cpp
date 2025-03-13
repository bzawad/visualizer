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
    // Use a wider field of view for more dramatic perspective (80 degrees)
    gluPerspective(80.0f, aspect, 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Position camera lower and closer to the road for a driver's perspective
    gluLookAt(0.0f, 0.6f, 1.8f,  // Eye position (lower and closer)
              0.0f, 0.1f, -5.0f, // Look target (looking further down the road)
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
        if (building.zPos > 3.0f) // Match the new near point
        {
            building.zPos = -5.0f; // Match the new far point

            // Recalculate x-position based on new z-position
            float t = (building.zPos - 3.0f) / (-5.0f - 3.0f);
            float roadWidthAtZ = ROAD_WIDTH * 2.5f * (1.0f - t) + ROAD_WIDTH * 0.9f * t;
            building.xPos = -(roadWidthAtZ + 0.2f); // Offset from road edge
        }

        // Calculate sine wave height with audio amplitude
        float wave = std::sin(building.zPos * SINE_FREQ + roadPosition) * audioAmplitude;
        building.height = BUILDING_HEIGHT * (1.0f + wave * 1.5f); // Increased audio response

        // Continuously update x-position to match road width at current z
        float t = (building.zPos - 3.0f) / (-5.0f - 3.0f);
        float roadWidthAtZ = ROAD_WIDTH * 2.5f * (1.0f - t) + ROAD_WIDTH * 0.9f * t;
        building.xPos = -(roadWidthAtZ + 0.2f); // Offset from road edge
    }

    for (auto &building : rightBuildings)
    {
        building.zPos += deltaTime * MOVE_SPEED;
        if (building.zPos > 3.0f) // Match the new near point
        {
            building.zPos = -5.0f; // Match the new far point

            // Recalculate x-position based on new z-position
            float t = (building.zPos - 3.0f) / (-5.0f - 3.0f);
            float roadWidthAtZ = ROAD_WIDTH * 2.5f * (1.0f - t) + ROAD_WIDTH * 0.9f * t;
            building.xPos = roadWidthAtZ + 0.2f; // Offset from road edge
        }

        // Calculate sine wave height with audio amplitude
        float wave = std::sin(building.zPos * SINE_FREQ + roadPosition) * audioAmplitude;
        building.height = BUILDING_HEIGHT * (1.0f + wave * 1.5f); // Increased audio response

        // Continuously update x-position to match road width at current z
        float t = (building.zPos - 3.0f) / (-5.0f - 3.0f);
        float roadWidthAtZ = ROAD_WIDTH * 2.5f * (1.0f - t) + ROAD_WIDTH * 0.9f * t;
        building.xPos = roadWidthAtZ + 0.2f; // Offset from road edge
    }
}

void RacerVisualizer::renderRoad()
{
    glLineWidth(2.0f);

    // Draw road with extended perspective
    // Near points should reach bottom corners of screen
    float nearZ = 3.0f; // Extend road closer to viewer
    float farZ = -5.0f; // Extend road further into the distance

    // Calculate wider width for near end to create stronger perspective
    float nearWidth = ROAD_WIDTH * 2.5f; // Wider at the near end
    float farWidth = ROAD_WIDTH * 0.9f;  // Slightly narrower at far end to enhance perspective

    // Draw road edges
    glColor3fv(ROAD_COLOR);
    glBegin(GL_LINES);
    // Left edge
    glVertex3f(-nearWidth, 0.0f, nearZ);
    glVertex3f(-farWidth, 0.0f, farZ);
    // Right edge
    glVertex3f(nearWidth, 0.0f, nearZ);
    glVertex3f(farWidth, 0.0f, farZ);
    glEnd();

    // Draw horizontal grid lines
    glColor3fv(GRID_COLOR);
    glBegin(GL_LINES);
    for (float z : roadLines)
    {
        // Map z from [-1,1] to [nearZ,farZ]
        float mappedZ = nearZ + (z + 1.0f) * (farZ - nearZ) / 2.0f;

        // Calculate width at this z position (linear interpolation)
        float t = (mappedZ - nearZ) / (farZ - nearZ);
        float width = nearWidth * (1.0f - t) + farWidth * t;

        glVertex3f(-width, 0.0f, mappedZ);
        glVertex3f(width, 0.0f, mappedZ);
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

void RacerVisualizer::renderSun()
{
    // Enable blending for glow effect
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Calculate the apparent width of the road at the horizon point
    // The road is at z = SUN_Z_POS and has width ROAD_WIDTH
    // Get the horizon point Y position in view space
    float horizonY = 0.0f; // The Y coordinate of the horizon

    // Draw sun at horizon, matching the road width
    glPushMatrix();
    // Position the sun exactly at the horizon point where the road ends
    glTranslatef(0.0f, horizonY, SUN_Z_POS);

    // Draw half-circle (180 degrees, only upper half)
    glBegin(GL_TRIANGLE_FAN);
    // Center point - use inner color
    glColor3fv(SUN_INNER_COLOR);
    glVertex3f(0.0f, 0.0f, 0.0f);

    // Outer points - use outer color
    glColor3fv(SUN_OUTER_COLOR);
    for (int i = 0; i <= SUN_SEGMENTS; i++)
    {
        float angle = M_PI * i / SUN_SEGMENTS; // 0 to π (180 degrees)
        float x = SUN_RADIUS * cos(angle);
        float y = SUN_RADIUS * sin(angle);
        // Only draw points in the upper half (y >= 0)
        if (y >= 0.0f)
        {
            glVertex3f(x, y, 0.0f);
        }
    }
    glEnd();

    // Draw sun "rays" or glow lines
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glColor4f(1.0f, 0.4f, 0.8f, 0.5f); // Semi-transparent pink

    // Draw rays around the half-circle
    for (int i = 0; i < 12; i++)
    {
        float angle = M_PI * i / 11; // 0 to π (180 degrees)
        if (sin(angle) >= 0)
        { // Only upper half
            float x1 = SUN_RADIUS * cos(angle);
            float y1 = SUN_RADIUS * sin(angle);
            float x2 = (SUN_RADIUS * 1.3f) * cos(angle);
            float y2 = (SUN_RADIUS * 1.3f) * sin(angle);

            glVertex3f(x1, y1, 0.0f);
            glVertex3f(x2, y2, 0.0f);
        }
    }
    glEnd();

    glPopMatrix();
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

    // Draw background sun first (it's furthest away)
    renderSun();

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

    // Draw background sun first (it's furthest away)
    renderSun();

    // Update and render components
    updateRoad(1.0f / 60.0f);
    updateBuildings(1.0f / 60.0f);
    renderRoad();
    renderBuildings();

    // Disable features
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
}