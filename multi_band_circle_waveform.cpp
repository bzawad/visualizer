#include "multi_band_circle_waveform.h"
#include <GL/glew.h>
#include <cmath>
#include <algorithm>

// Define the static constexpr color arrays
constexpr float MultiBandCircleWaveform::LOW_COLOR[3];
constexpr float MultiBandCircleWaveform::MID_COLOR[3];
constexpr float MultiBandCircleWaveform::HIGH_COLOR[3];

MultiBandCircleWaveform::MultiBandCircleWaveform()
{
}

MultiBandCircleWaveform::~MultiBandCircleWaveform()
{
}

void MultiBandCircleWaveform::renderFrame(const std::vector<float> &audioData,
                                          double *fftInputBuffer,
                                          fftw_complex *fftOutputBuffer,
                                          fftw_plan &fftPlan,
                                          float timeSeconds)
{
    // Calculate the sample index for the current time
    size_t sampleIndex = static_cast<size_t>(timeSeconds * 44100); // Assuming 44.1kHz
    if (sampleIndex >= audioData.size())
        return;

    // Fill the FFT input buffer with samples at this time
    for (int i = 0; i < N; i++)
    {
        if (sampleIndex + i < audioData.size())
        {
            fftInputBuffer[i] = audioData[sampleIndex + i];
        }
        else
        {
            fftInputBuffer[i] = 0.0;
        }
    }

    // Execute FFT
    fftw_execute(fftPlan);

    // Filter audio into frequency bands with adjusted scaling
    std::vector<float> lowBand = filterBand(fftOutputBuffer, 0, (LOW_CUTOFF * N) / 44100, 1.0f); // Normal low frequencies
    std::vector<float> midBand = filterBand(fftOutputBuffer, (LOW_CUTOFF * N) / 44100, (MID_CUTOFF * N) / 44100, 2.0f);
    std::vector<float> highBand = filterBand(fftOutputBuffer, (MID_CUTOFF * N) / 44100, (HIGH_CUTOFF * N) / 44100, 3.0f); // Boost high frequencies more

    // Render each band as a circle with increased thickness
    renderCircularBand(lowBand, LOW_RADIUS, THICKNESS * 1.5f, LOW_COLOR);
    renderCircularBand(midBand, MID_RADIUS, THICKNESS * 1.5f, MID_COLOR);
    renderCircularBand(highBand, HIGH_RADIUS, THICKNESS * 1.5f, HIGH_COLOR);
}

void MultiBandCircleWaveform::renderLiveFrame(const std::vector<float> &, // audioData (unused)
                                              double *,                   // fftInputBuffer (unused)
                                              fftw_complex *fftOutputBuffer,
                                              fftw_plan &fftPlan,
                                              size_t) // currentPosition (unused)
{
    // Execute FFT (input buffer should already be updated)
    fftw_execute(fftPlan);

    // Filter audio into frequency bands with adjusted scaling
    std::vector<float> lowBand = filterBand(fftOutputBuffer, 0, (LOW_CUTOFF * N) / 44100, 1.0f); // Normal low frequencies
    std::vector<float> midBand = filterBand(fftOutputBuffer, (LOW_CUTOFF * N) / 44100, (MID_CUTOFF * N) / 44100, 2.0f);
    std::vector<float> highBand = filterBand(fftOutputBuffer, (MID_CUTOFF * N) / 44100, (HIGH_CUTOFF * N) / 44100, 3.0f); // Boost high frequencies more

    // Render each band as a circle with increased thickness
    renderCircularBand(lowBand, LOW_RADIUS, THICKNESS * 1.5f, LOW_COLOR);
    renderCircularBand(midBand, MID_RADIUS, THICKNESS * 1.5f, MID_COLOR);
    renderCircularBand(highBand, HIGH_RADIUS, THICKNESS * 1.5f, HIGH_COLOR);
}

void MultiBandCircleWaveform::renderCircularBand(const std::vector<float> &bandData, float radius, float thickness, const float *color)
{
    const int numPoints = 100; // Number of points to draw the circle
    const float twoPi = 2.0f * M_PI;

    glColor3fv(color);
    glLineWidth(5.0f); // Increased line width to 5 pixels
    glBegin(GL_LINE_STRIP);

    for (int i = 0; i <= numPoints; i++)
    {
        float angle = (i * twoPi) / numPoints;
        float sampleIndex = (i * static_cast<float>(bandData.size())) / numPoints;
        int index = static_cast<int>(sampleIndex);

        // Get amplitude from band data
        float amplitude = (index < static_cast<int>(bandData.size())) ? bandData[index] * thickness : 0.0f;

        // Calculate position with amplitude modulation
        float r = radius + amplitude;
        float x = r * cos(angle);
        float y = r * sin(angle);

        glVertex2f(x, y);
    }

    glEnd();
    glLineWidth(1.0f); // Reset line width
}

std::vector<float> MultiBandCircleWaveform::filterBand(fftw_complex *fftOutput, int startBin, int endBin, float bandScaling)
{
    std::vector<float> bandData;
    startBin = std::max(0, std::min(startBin, N / 2));
    endBin = std::max(0, std::min(endBin, N / 2));

    // Process each FFT bin in the frequency range
    for (int i = startBin; i < endBin; i++)
    {
        // Calculate magnitude
        float magnitude = std::sqrt(fftOutput[i][0] * fftOutput[i][0] +
                                    fftOutput[i][1] * fftOutput[i][1]);

        // Apply frequency-dependent scaling
        float freqScaling = std::pow(static_cast<float>(i) / (startBin + 1), 0.5f);
        magnitude *= freqScaling * bandScaling; // Apply additional band-specific scaling

        // Normalize with reduced divisor for stronger response
        magnitude = std::min(1.0f, magnitude / 12.5f); // Reduced from 25.0f to 12.5f for stronger response

        bandData.push_back(magnitude);
    }

    return bandData;
}