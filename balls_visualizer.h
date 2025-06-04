#pragma once

#include "visualizer_base.h"
#include <vector>
#include <array>
#include <random>

struct Ball
{
    float x, y;            // Position
    float vx, vy;          // Velocity
    float radius;          // Size
    float color[3];        // RGB color
    float energy;          // Energy level for audio reactivity
    int frequencyBand;     // Which frequency band this ball responds to
    float bounceIntensity; // How much it bounces based on audio
};

class BallsVisualizer : public Visualizer
{
public:
    BallsVisualizer();
    ~BallsVisualizer() override = default;

    void initialize(int width, int height) override;

    void renderFrame(const std::vector<float> &audioData,
                     double *in,
                     fftw_complex *out,
                     fftw_plan &plan,
                     float timeSeconds) override;

    void renderLiveFrame(const std::vector<float> &audioData,
                         double *in,
                         fftw_complex *out,
                         fftw_plan &plan,
                         size_t currentPosition) override;

private:
    void render(float time, const std::vector<float> &magnitudes);
    void updateBalls(float deltaTime, const std::vector<float> &magnitudes);
    void drawBall(const Ball &ball);
    std::vector<float> calculateMagnitudes(fftw_complex *out);
    void initializeBalls();

    std::vector<Ball> balls;
    std::mt19937 rng;
    float aspectRatio;
    float lastTime;

    // Physics parameters
    static constexpr float GRAVITY = 0.5f;
    static constexpr float DAMPING = 0.98f;
    static constexpr float BOUNCE_DAMPING = 0.85f;
    static constexpr float MIN_VELOCITY = 0.01f;
    static constexpr float MAX_VELOCITY = 3.0f;

    // Audio reactivity parameters
    static constexpr float BASE_BOUNCE_FORCE = 0.3f;
    static constexpr float MAX_BOUNCE_FORCE = 2.5f;
    static constexpr float ENERGY_DECAY = 0.95f;
    static constexpr float AUDIO_SENSITIVITY = 8.0f;

    // Visual parameters
    static constexpr int NUM_BALLS = 12;
    static constexpr float MIN_RADIUS = 0.02f;
    static constexpr float MAX_RADIUS = 0.08f;
    static constexpr int BALL_SEGMENTS = 16;

    // Frequency band divisions
    static constexpr int NUM_FREQUENCY_BANDS = 6;
    static constexpr int BAND_SIZE = 8; // How many FFT bins per band
};