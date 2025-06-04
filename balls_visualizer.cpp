#include "balls_visualizer.h"
#include <GL/glew.h>
#include <cmath>
#include <algorithm>

BallsVisualizer::BallsVisualizer() : aspectRatio(1.0f), lastTime(0.0f)
{
    std::random_device rd;
    rng.seed(rd());
}

void BallsVisualizer::initialize(int width, int height)
{
    Visualizer::initialize(width, height);
    aspectRatio = static_cast<float>(width) / height;

    // Set background to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Enable blending for smooth circles
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initializeBalls();
}

void BallsVisualizer::initializeBalls()
{
    balls.clear();
    balls.reserve(NUM_BALLS);

    std::uniform_real_distribution<float> xDist(-0.8f, 0.8f);
    std::uniform_real_distribution<float> yDist(-0.5f, 0.8f);
    std::uniform_real_distribution<float> vxDist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> vyDist(-0.5f, 1.5f);
    std::uniform_real_distribution<float> radiusDist(MIN_RADIUS, MAX_RADIUS);
    std::uniform_real_distribution<float> colorDist(0.3f, 1.0f);
    std::uniform_int_distribution<int> bandDist(0, NUM_FREQUENCY_BANDS - 1);

    // Generate colorful balls with varied properties
    for (int i = 0; i < NUM_BALLS; i++)
    {
        Ball ball;
        ball.x = xDist(rng);
        ball.y = yDist(rng);
        ball.vx = vxDist(rng);
        ball.vy = vyDist(rng);
        ball.radius = radiusDist(rng);
        ball.energy = 0.0f;
        ball.frequencyBand = bandDist(rng);
        ball.bounceIntensity = 0.0f;

        // Create rainbow-like colors based on ball index
        float hue = static_cast<float>(i) / NUM_BALLS * 360.0f;
        float saturation = 0.8f + colorDist(rng) * 0.2f;
        float value = 0.7f + colorDist(rng) * 0.3f;

        // Convert HSV to RGB
        float c = value * saturation;
        float x = c * (1.0f - std::abs(std::fmod(hue / 60.0f, 2.0f) - 1.0f));
        float m = value - c;

        if (hue < 60)
        {
            ball.color[0] = c + m;
            ball.color[1] = x + m;
            ball.color[2] = m;
        }
        else if (hue < 120)
        {
            ball.color[0] = x + m;
            ball.color[1] = c + m;
            ball.color[2] = m;
        }
        else if (hue < 180)
        {
            ball.color[0] = m;
            ball.color[1] = c + m;
            ball.color[2] = x + m;
        }
        else if (hue < 240)
        {
            ball.color[0] = m;
            ball.color[1] = x + m;
            ball.color[2] = c + m;
        }
        else if (hue < 300)
        {
            ball.color[0] = x + m;
            ball.color[1] = m;
            ball.color[2] = c + m;
        }
        else
        {
            ball.color[0] = c + m;
            ball.color[1] = m;
            ball.color[2] = x + m;
        }

        balls.push_back(ball);
    }
}

void BallsVisualizer::renderFrame(const std::vector<float> &audioData,
                                  double *in,
                                  fftw_complex *out,
                                  fftw_plan &plan,
                                  float timeSeconds)
{
    // Process audio data for FFT
    size_t numSamples = std::min(static_cast<size_t>(N), audioData.size());
    for (size_t i = 0; i < numSamples; i++)
    {
        // Apply Hanning window
        double multiplier = 0.5 * (1 - cos(2 * M_PI * i / (N - 1)));
        in[i] = audioData[i] * multiplier;
    }

    // Execute FFT
    fftw_execute(plan);

    std::vector<float> magnitudes = calculateMagnitudes(out);
    render(timeSeconds, magnitudes);
}

void BallsVisualizer::renderLiveFrame(const std::vector<float> &audioData,
                                      double *in,
                                      fftw_complex *out,
                                      fftw_plan &plan,
                                      size_t currentPosition)
{
    // Process audio data for FFT
    size_t numSamples = std::min(static_cast<size_t>(N), audioData.size());
    for (size_t i = 0; i < numSamples; i++)
    {
        size_t index = (currentPosition + i) % audioData.size();
        // Apply Hanning window
        double multiplier = 0.5 * (1 - cos(2 * M_PI * i / (N - 1)));
        in[i] = audioData[index] * multiplier;
    }

    // Execute FFT
    fftw_execute(plan);

    float timeSeconds = static_cast<float>(currentPosition) / 44100.0f;
    std::vector<float> magnitudes = calculateMagnitudes(out);
    render(timeSeconds, magnitudes);
}

std::vector<float> BallsVisualizer::calculateMagnitudes(fftw_complex *out)
{
    std::vector<float> magnitudes(N / 2);
    for (size_t i = 0; i < N / 2; i++)
    {
        magnitudes[i] = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]) / N;
    }
    return magnitudes;
}

void BallsVisualizer::render(float time, const std::vector<float> &magnitudes)
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Calculate delta time
    float deltaTime = (lastTime > 0.0f) ? (time - lastTime) : (1.0f / 60.0f);
    deltaTime = std::min(deltaTime, 1.0f / 30.0f); // Cap at 30 FPS minimum
    lastTime = time;

    // Set up 2D rendering
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0f * aspectRatio, 1.0f * aspectRatio, -1.0f, 1.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Update and render balls
    updateBalls(deltaTime, magnitudes);

    for (const auto &ball : balls)
    {
        drawBall(ball);
    }
}

void BallsVisualizer::updateBalls(float deltaTime, const std::vector<float> &magnitudes)
{
    // Calculate frequency band energies
    std::vector<float> bandEnergies(NUM_FREQUENCY_BANDS, 0.0f);
    size_t maxBin = std::min(static_cast<size_t>(NUM_FREQUENCY_BANDS * BAND_SIZE), magnitudes.size());

    for (size_t i = 0; i < maxBin; i++)
    {
        int band = i / BAND_SIZE;
        if (band < NUM_FREQUENCY_BANDS)
        {
            bandEnergies[band] += magnitudes[i];
        }
    }

    // Normalize band energies
    for (int i = 0; i < NUM_FREQUENCY_BANDS; i++)
    {
        bandEnergies[i] = std::min(1.0f, bandEnergies[i] * AUDIO_SENSITIVITY);
    }

    // Update each ball
    for (auto &ball : balls)
    {
        // Apply gravity
        ball.vy -= GRAVITY * deltaTime;

        // Get audio energy for this ball's frequency band
        float audioEnergy = 0.0f;
        if (ball.frequencyBand < static_cast<int>(bandEnergies.size()))
        {
            audioEnergy = bandEnergies[ball.frequencyBand];
        }

        // Update energy with decay
        ball.energy = ball.energy * ENERGY_DECAY + audioEnergy * (1.0f - ENERGY_DECAY);

        // Calculate bounce intensity
        ball.bounceIntensity = BASE_BOUNCE_FORCE + ball.energy * (MAX_BOUNCE_FORCE - BASE_BOUNCE_FORCE);

        // Update position
        ball.x += ball.vx * deltaTime;
        ball.y += ball.vy * deltaTime;

        // Apply damping
        ball.vx *= DAMPING;
        ball.vy *= DAMPING;

        // Bounce off walls (accounting for aspect ratio)
        float maxX = aspectRatio - ball.radius;
        float minX = -aspectRatio + ball.radius;

        if (ball.x > maxX)
        {
            ball.x = maxX;
            ball.vx = -std::abs(ball.vx) * BOUNCE_DAMPING;
            // Audio-reactive bounce
            ball.vy += ball.bounceIntensity * audioEnergy;
        }
        else if (ball.x < minX)
        {
            ball.x = minX;
            ball.vx = std::abs(ball.vx) * BOUNCE_DAMPING;
            // Audio-reactive bounce
            ball.vy += ball.bounceIntensity * audioEnergy;
        }

        // Bounce off floor and ceiling
        if (ball.y > (1.0f - ball.radius))
        {
            ball.y = 1.0f - ball.radius;
            ball.vy = -std::abs(ball.vy) * BOUNCE_DAMPING;
            // Audio-reactive bounce
            ball.vy -= ball.bounceIntensity * audioEnergy;
        }
        else if (ball.y < (-1.0f + ball.radius))
        {
            ball.y = -1.0f + ball.radius;
            ball.vy = std::abs(ball.vy) * BOUNCE_DAMPING;
            // Strong audio-reactive bounce from floor
            ball.vy += ball.bounceIntensity * audioEnergy * 1.5f;
        }

        // Clamp velocities to prevent extreme speeds
        ball.vx = std::max(-MAX_VELOCITY, std::min(MAX_VELOCITY, ball.vx));
        ball.vy = std::max(-MAX_VELOCITY, std::min(MAX_VELOCITY, ball.vy));

        // Stop very slow movement
        if (std::abs(ball.vx) < MIN_VELOCITY)
            ball.vx = 0.0f;
        if (std::abs(ball.vy) < MIN_VELOCITY)
            ball.vy = 0.0f;
    }
}

void BallsVisualizer::drawBall(const Ball &ball)
{
    glPushMatrix();
    glTranslatef(ball.x, ball.y, 0.0f);

    // Make balls glow based on their energy level
    float intensity = 0.6f + ball.energy * 0.4f;
    glColor4f(ball.color[0] * intensity,
              ball.color[1] * intensity,
              ball.color[2] * intensity,
              0.9f);

    // Draw filled circle
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.0f, 0.0f); // Center

    for (int i = 0; i <= BALL_SEGMENTS; i++)
    {
        float angle = static_cast<float>(i) * 2.0f * M_PI / BALL_SEGMENTS;
        float x = ball.radius * cos(angle);
        float y = ball.radius * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();

    // Draw bright outline for extra glow effect
    if (ball.energy > 0.3f)
    {
        glColor4f(ball.color[0], ball.color[1], ball.color[2], ball.energy * 0.7f);
        glLineWidth(2.0f);

        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < BALL_SEGMENTS; i++)
        {
            float angle = static_cast<float>(i) * 2.0f * M_PI / BALL_SEGMENTS;
            float x = ball.radius * cos(angle);
            float y = ball.radius * sin(angle);
            glVertex2f(x, y);
        }
        glEnd();

        glLineWidth(1.0f);
    }

    glPopMatrix();
}