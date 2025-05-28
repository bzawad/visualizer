#include "maze_visualizer.h"
#include <GL/glew.h>
#include <cmath>
#include <algorithm>
#include <random>
#include <vector>

#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

MazeVisualizer::MazeVisualizer() : audioAmplitude(0.0f), mazePosition(0.0f), cameraRotation(0.0f)
{
    generateMaze();

    // Initialize tunnel path
    for (int i = 0; i < 50; i++)
    {
        float angle = i * 0.2f;
        float x = std::sin(angle) * 3.0f;
        float z = -i * 0.5f;
        createTunnelSegment(x, z, angle);
    }
}

MazeVisualizer::~MazeVisualizer() {}

void MazeVisualizer::initialize(int width, int height)
{
    Visualizer::initialize(width, height);

    // Enable depth testing for 3D
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Set background to black for vector effect
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void MazeVisualizer::generateMaze()
{
    maze.resize(MAZE_SIZE, std::vector<MazeCell>(MAZE_SIZE));

    // Initialize all cells as walls
    for (int x = 0; x < MAZE_SIZE; x++)
    {
        for (int z = 0; z < MAZE_SIZE; z++)
        {
            maze[x][z] = {true, WALL_HEIGHT, 0.5f};
        }
    }

    // Use recursive backtracking to generate a proper maze
    std::vector<std::vector<bool>> visited(MAZE_SIZE, std::vector<bool>(MAZE_SIZE, false));
    std::vector<std::pair<int, int>> stack;

    // Start from center
    int startX = MAZE_SIZE / 2;
    int startZ = MAZE_SIZE / 2;

    // Make sure start is odd coordinates for proper maze generation
    if (startX % 2 == 0)
        startX++;
    if (startZ % 2 == 0)
        startZ++;

    // Mark starting cell as path
    maze[startX][startZ] = {false, 0.0f, 0.5f};
    visited[startX][startZ] = true;
    stack.push_back({startX, startZ});

    // Directions: North, South, East, West (2 cells away for proper maze)
    int dx[] = {0, 0, 2, -2};
    int dz[] = {-2, 2, 0, 0};

    std::random_device rd;
    std::mt19937 gen(rd());

    while (!stack.empty())
    {
        auto [currentX, currentZ] = stack.back();

        // Find unvisited neighbors
        std::vector<int> neighbors;
        for (int i = 0; i < 4; i++)
        {
            int newX = currentX + dx[i];
            int newZ = currentZ + dz[i];

            // Check bounds and if unvisited
            if (newX > 0 && newX < MAZE_SIZE - 1 &&
                newZ > 0 && newZ < MAZE_SIZE - 1 &&
                !visited[newX][newZ])
            {
                neighbors.push_back(i);
            }
        }

        if (!neighbors.empty())
        {
            // Choose random neighbor
            std::uniform_int_distribution<> dis(0, neighbors.size() - 1);
            int direction = neighbors[dis(gen)];

            int newX = currentX + dx[direction];
            int newZ = currentZ + dz[direction];

            // Create path to neighbor
            maze[newX][newZ] = {false, 0.0f, 0.5f};
            visited[newX][newZ] = true;

            // Remove wall between current and neighbor
            int wallX = currentX + dx[direction] / 2;
            int wallZ = currentZ + dz[direction] / 2;
            maze[wallX][wallZ] = {false, 0.0f, 0.5f};

            // Add neighbor to stack
            stack.push_back({newX, newZ});
        }
        else
        {
            // Backtrack
            stack.pop_back();
        }
    }

    // Add some random openings to make it less perfect
    std::uniform_real_distribution<float> openingChance(0.0f, 1.0f);
    for (int x = 1; x < MAZE_SIZE - 1; x++)
    {
        for (int z = 1; z < MAZE_SIZE - 1; z++)
        {
            if (maze[x][z].hasWall && openingChance(gen) < 0.05f) // 5% chance
            {
                // Check if removing this wall connects two paths
                int pathNeighbors = 0;
                if (!maze[x - 1][z].hasWall)
                    pathNeighbors++;
                if (!maze[x + 1][z].hasWall)
                    pathNeighbors++;
                if (!maze[x][z - 1].hasWall)
                    pathNeighbors++;
                if (!maze[x][z + 1].hasWall)
                    pathNeighbors++;

                if (pathNeighbors >= 2)
                {
                    maze[x][z] = {false, 0.0f, 0.5f};
                }
            }
        }
    }

    // Ensure outer walls remain
    for (int i = 0; i < MAZE_SIZE; i++)
    {
        maze[0][i] = {true, WALL_HEIGHT, 0.5f};
        maze[MAZE_SIZE - 1][i] = {true, WALL_HEIGHT, 0.5f};
        maze[i][0] = {true, WALL_HEIGHT, 0.5f};
        maze[i][MAZE_SIZE - 1] = {true, WALL_HEIGHT, 0.5f};
    }
}

void MazeVisualizer::createTunnelSegment(float x, float z, float rotation)
{
    tunnelPath.push_back({x, z, rotation, TUNNEL_WIDTH});
}

void MazeVisualizer::setupPerspectiveView()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float aspect = static_cast<float>(screenWidth) / screenHeight;

#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
    gluPerspective(75.0f, aspect, 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Camera follows a path through the maze
    // Convert world position to maze coordinates
    float worldZ = mazePosition;
    int mazeZ = static_cast<int>((worldZ / CELL_SIZE) + MAZE_SIZE / 2);

    // Find a valid path to follow
    float cameraX = 0.0f;
    for (int x = 1; x < MAZE_SIZE - 1; x++)
    {
        int checkZ = mazeZ % MAZE_SIZE;
        if (checkZ >= 0 && checkZ < MAZE_SIZE && !maze[x][checkZ].hasWall)
        {
            cameraX = (x - MAZE_SIZE / 2) * CELL_SIZE;
            break;
        }
    }

    float cameraY = 0.4f + audioAmplitude * 0.1f; // Eye level with slight audio bob
    float cameraZ = mazePosition;

    // Look ahead in the direction of movement
    float targetX = cameraX;
    float targetY = 0.4f;
    float targetZ = mazePosition - 2.0f;

    gluLookAt(cameraX, cameraY, cameraZ, // Eye
              targetX, targetY, targetZ, // Target
              0.0f, 1.0f, 0.0f);         // Up
#ifdef __APPLE__
#pragma clang diagnostic pop
#endif
}

void MazeVisualizer::updateMaze(float deltaTime)
{
    mazePosition -= deltaTime * MOVE_SPEED;
    cameraRotation += deltaTime * 0.5f;

    // Update wall heights and glow based on audio
    for (int x = 0; x < MAZE_SIZE; x++)
    {
        for (int z = 0; z < MAZE_SIZE; z++)
        {
            if (maze[x][z].hasWall)
            {
                // Pulse wall height with audio
                float basePulse = std::sin((x + z) * 0.5f + mazePosition * 0.1f);
                maze[x][z].height = WALL_HEIGHT * (1.0f + audioAmplitude * PULSE_INTENSITY * basePulse);

                // Update glow intensity
                maze[x][z].glowIntensity = audioAmplitude * (0.5f + 0.5f * std::sin(mazePosition * 0.2f + x + z));
            }
        }
    }
}

void MazeVisualizer::updateTunnel(float deltaTime)
{
    // Move tunnel segments
    for (auto &segment : tunnelPath)
    {
        segment.z += deltaTime * MOVE_SPEED;

        // Wrap segments that have moved too far
        if (segment.z > 10.0f)
        {
            segment.z -= 60.0f; // Reset to back
            segment.x = std::sin(segment.z * 0.1f) * 3.0f;
            segment.rotation = segment.z * 0.02f;
        }

        // Pulse width with audio
        segment.width = TUNNEL_WIDTH * (1.0f + audioAmplitude * 0.3f);
    }
}

void MazeVisualizer::renderMazeWalls()
{
    glLineWidth(2.0f + audioAmplitude * 3.0f);

    glBegin(GL_LINES);

    for (int x = 0; x < MAZE_SIZE; x++)
    {
        for (int z = 0; z < MAZE_SIZE; z++)
        {
            if (!maze[x][z].hasWall)
                continue;

            float worldX = (x - MAZE_SIZE / 2) * CELL_SIZE;
            float worldZ = (z - MAZE_SIZE / 2) * CELL_SIZE + mazePosition;
            float height = maze[x][z].height;
            float glow = maze[x][z].glowIntensity;

            // Skip walls that are too far away
            if (worldZ < mazePosition - 20.0f || worldZ > mazePosition + 20.0f)
                continue;

            // Set color with glow effect
            glColor3f(WALL_COLOR[0] * (0.5f + glow * 0.5f),
                      WALL_COLOR[1] * (0.5f + glow * 0.5f),
                      WALL_COLOR[2] * (0.5f + glow * 0.5f));

            float cellHalf = CELL_SIZE * 0.5f;

            // Draw a solid cube-like wall structure
            // Bottom face outline
            glVertex3f(worldX - cellHalf, 0.0f, worldZ - cellHalf);
            glVertex3f(worldX + cellHalf, 0.0f, worldZ - cellHalf);

            glVertex3f(worldX + cellHalf, 0.0f, worldZ - cellHalf);
            glVertex3f(worldX + cellHalf, 0.0f, worldZ + cellHalf);

            glVertex3f(worldX + cellHalf, 0.0f, worldZ + cellHalf);
            glVertex3f(worldX - cellHalf, 0.0f, worldZ + cellHalf);

            glVertex3f(worldX - cellHalf, 0.0f, worldZ + cellHalf);
            glVertex3f(worldX - cellHalf, 0.0f, worldZ - cellHalf);

            // Top face outline
            glVertex3f(worldX - cellHalf, height, worldZ - cellHalf);
            glVertex3f(worldX + cellHalf, height, worldZ - cellHalf);

            glVertex3f(worldX + cellHalf, height, worldZ - cellHalf);
            glVertex3f(worldX + cellHalf, height, worldZ + cellHalf);

            glVertex3f(worldX + cellHalf, height, worldZ + cellHalf);
            glVertex3f(worldX - cellHalf, height, worldZ + cellHalf);

            glVertex3f(worldX - cellHalf, height, worldZ + cellHalf);
            glVertex3f(worldX - cellHalf, height, worldZ - cellHalf);

            // Vertical edges connecting bottom to top
            glVertex3f(worldX - cellHalf, 0.0f, worldZ - cellHalf);
            glVertex3f(worldX - cellHalf, height, worldZ - cellHalf);

            glVertex3f(worldX + cellHalf, 0.0f, worldZ - cellHalf);
            glVertex3f(worldX + cellHalf, height, worldZ - cellHalf);

            glVertex3f(worldX - cellHalf, 0.0f, worldZ + cellHalf);
            glVertex3f(worldX - cellHalf, height, worldZ + cellHalf);

            glVertex3f(worldX + cellHalf, 0.0f, worldZ + cellHalf);
            glVertex3f(worldX + cellHalf, height, worldZ + cellHalf);

            // Add some internal structure for more detail
            // Cross pattern on front face
            glVertex3f(worldX - cellHalf, height * 0.5f, worldZ - cellHalf);
            glVertex3f(worldX + cellHalf, height * 0.5f, worldZ - cellHalf);

            glVertex3f(worldX, 0.0f, worldZ - cellHalf);
            glVertex3f(worldX, height, worldZ - cellHalf);
        }
    }

    glEnd();
}

void MazeVisualizer::renderFloorAndCeiling()
{
    glLineWidth(1.0f);

    // Floor grid
    glBegin(GL_LINES);
    glColor3fv(FLOOR_COLOR);

    float gridSize = MAZE_SIZE * CELL_SIZE;
    float gridStep = CELL_SIZE;

    for (float x = -gridSize; x <= gridSize; x += gridStep)
    {
        glVertex3f(x, 0.0f, mazePosition - 20.0f);
        glVertex3f(x, 0.0f, mazePosition + 20.0f);
    }

    for (float z = mazePosition - 20.0f; z <= mazePosition + 20.0f; z += gridStep)
    {
        glVertex3f(-gridSize, 0.0f, z);
        glVertex3f(gridSize, 0.0f, z);
    }

    glEnd();

    // Ceiling grid (higher up)
    glBegin(GL_LINES);
    glColor3fv(CEILING_COLOR);

    float ceilingHeight = 3.0f + audioAmplitude * 0.5f;

    for (float x = -gridSize; x <= gridSize; x += gridStep * 2)
    {
        glVertex3f(x, ceilingHeight, mazePosition - 20.0f);
        glVertex3f(x, ceilingHeight, mazePosition + 20.0f);
    }

    for (float z = mazePosition - 20.0f; z <= mazePosition + 20.0f; z += gridStep * 2)
    {
        glVertex3f(-gridSize, ceilingHeight, z);
        glVertex3f(gridSize, ceilingHeight, z);
    }

    glEnd();
}

void MazeVisualizer::renderTunnelEffects()
{
    // Render a simple path line through the maze corridors
    glLineWidth(2.0f + audioAmplitude * 3.0f);

    glBegin(GL_LINE_STRIP);
    glColor3f(GLOW_COLOR[0] * (0.5f + audioAmplitude * 0.5f),
              GLOW_COLOR[1] * (0.5f + audioAmplitude * 0.5f),
              GLOW_COLOR[2] * (0.5f + audioAmplitude * 0.5f));

    // Draw a path line at floor level through open corridors
    for (float z = mazePosition - 15.0f; z <= mazePosition + 5.0f; z += 0.5f)
    {
        int mazeZ = static_cast<int>((z / CELL_SIZE) + MAZE_SIZE / 2);
        if (mazeZ >= 0 && mazeZ < MAZE_SIZE)
        {
            // Find an open path at this Z level
            for (int x = 1; x < MAZE_SIZE - 1; x++)
            {
                if (!maze[x][mazeZ].hasWall)
                {
                    float worldX = (x - MAZE_SIZE / 2) * CELL_SIZE;
                    glVertex3f(worldX, 0.05f, z);
                    break;
                }
            }
        }
    }

    glEnd();
}

float MazeVisualizer::calculateAudioAmplitude(const std::vector<float> &audioData, size_t position)
{
    const size_t windowSize = 1024;
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

    return std::min(1.0f, average * 5.0f); // Amplify for dramatic effect
}

void MazeVisualizer::renderFrame(const std::vector<float> &audioData,
                                 double * /* fftInputBuffer */,
                                 fftw_complex * /* fftOutputBuffer */,
                                 fftw_plan & /* fftPlan */,
                                 float timeSeconds)
{
    size_t sampleIndex = static_cast<size_t>(timeSeconds * 44100);
    audioAmplitude = calculateAudioAmplitude(audioData, sampleIndex);

    setupPerspectiveView();

    // Enable line smoothing for vector effect
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update and render
    updateMaze(1.0f / 60.0f);
    updateTunnel(1.0f / 60.0f);

    renderFloorAndCeiling();
    renderMazeWalls();
    renderTunnelEffects();

    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
}

void MazeVisualizer::renderLiveFrame(const std::vector<float> &audioData,
                                     double * /* fftInputBuffer */,
                                     fftw_complex * /* fftOutputBuffer */,
                                     fftw_plan & /* fftPlan */,
                                     size_t currentPosition)
{
    audioAmplitude = calculateAudioAmplitude(audioData, currentPosition);

    setupPerspectiveView();

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    updateMaze(1.0f / 60.0f);
    updateTunnel(1.0f / 60.0f);

    renderFloorAndCeiling();
    renderMazeWalls();
    renderTunnelEffects();

    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
}