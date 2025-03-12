#include "ascii_bar_equalizer.h"
#include <cmath>
#include <algorithm>
#include <chrono>

AsciiBarEqualizer::AsciiBarEqualizer(int numBars)
    : numBars(numBars),
      rng(std::chrono::system_clock::now().time_since_epoch().count()),
      dist(0, 1) // Distribution for random 0s and 1s
{
}

AsciiBarEqualizer::~AsciiBarEqualizer()
{
}

void AsciiBarEqualizer::renderFrame(const std::vector<float> &audioData,
                                    double *fftInputBuffer,
                                    fftw_complex *fftOutputBuffer,
                                    fftw_plan &fftPlan,
                                    float timeSeconds)
{
    // Set color for visualization
    glColor3f(0.0f, 1.0f, 0.0f); // Green visualization

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

    // Render bars based on FFT output
    renderBars(fftOutputBuffer);
}

void AsciiBarEqualizer::renderLiveFrame(const std::vector<float> &audioData,
                                        double *fftInputBuffer,
                                        fftw_complex *fftOutputBuffer,
                                        fftw_plan &fftPlan,
                                        size_t currentPosition)
{
    // Mark unused parameters to silence compiler warnings
    (void)audioData;
    (void)fftInputBuffer;
    (void)currentPosition;

    // Execute FFT
    fftw_execute(fftPlan);

    // Render bars based on FFT output
    renderBars(fftOutputBuffer);
}

void AsciiBarEqualizer::renderBars(fftw_complex *fftOutputBuffer)
{
    // Calculate frequency bands for bars using logarithmic scale
    float barWidth = 2.0f / numBars; // normalized width in [-1, 1] space

    // Pre-calculate frequency scaling factors
    const float minFreq = 20.0f;    // 20 Hz
    const float maxFreq = 20000.0f; // 20 kHz
    const float freqRange = std::log10(maxFreq / minFreq);

    for (int i = 0; i < numBars; i++)
    {
        // Calculate frequency range for this bar using logarithmic scale
        float f1 = minFreq * std::pow(10.0f, (freqRange * i) / numBars);
        float f2 = minFreq * std::pow(10.0f, (freqRange * (i + 1)) / numBars);

        // Convert frequencies to FFT bin indices
        int startIdx = static_cast<int>((f1 * N) / 44100.0f);
        int endIdx = static_cast<int>((f2 * N) / 44100.0f);

        // Clamp indices to valid range
        startIdx = std::max(0, std::min(startIdx, N / 2 - 1));
        endIdx = std::max(0, std::min(endIdx, N / 2 - 1));
        endIdx = std::max(endIdx, startIdx + 1); // Ensure at least one bin

        // Calculate average amplitude in this frequency range
        float sum = 0.0f;
        for (int j = startIdx; j <= endIdx; j++)
        {
            float magnitude = std::sqrt(fftOutputBuffer[j][0] * fftOutputBuffer[j][0] +
                                        fftOutputBuffer[j][1] * fftOutputBuffer[j][1]);

            // Apply frequency-dependent scaling
            float freqScaling = std::pow(static_cast<float>(j) / startIdx, 0.5f); // Square root scaling
            magnitude *= freqScaling;

            sum += magnitude;
        }
        float avg = sum / (endIdx - startIdx + 1);

        // Apply additional scaling based on frequency band
        float bandScaling = 1.0f + (static_cast<float>(i) / numBars) * 2.0f; // Linear scaling with frequency
        avg *= bandScaling;

        // Normalize and apply some scaling for better visualization
        float height = std::min(1.0f, avg / 25.0f); // Adjusted base scaling

        // Calculate bar position
        float xLeft = -1.0f + i * barWidth;
        float xRight = xLeft + barWidth * 0.8f; // Small gap between bars

        // Render ASCII bar
        renderAsciiBar(xLeft, xRight, height * 2.0f); // Scale height to fill range
    }
}

void AsciiBarEqualizer::renderAsciiBar(float xLeft, float xRight, float height)
{
    const int CHARS_PER_BAR = 8;        // Number of characters horizontally per bar
    const int VERTICAL_RESOLUTION = 20; // Number of possible vertical positions

    float charWidth = (xRight - xLeft) / CHARS_PER_BAR;
    float charHeight = 2.0f / VERTICAL_RESOLUTION; // Total range is 2.0 (-1 to 1)

    int numCharsVertical = static_cast<int>(height * VERTICAL_RESOLUTION / 2);

    // For each character position in the bar
    for (int x = 0; x < CHARS_PER_BAR; x++)
    {
        for (int y = 0; y < numCharsVertical; y++)
        {
            // Generate random 0 or 1
            bool isOne = dist(rng) == 1;

            // Calculate character position
            float charX = xLeft + x * charWidth;
            float charY = -1.0f + y * charHeight;

            // Draw the character
            glBegin(GL_QUADS);
            if (isOne)
            {
                // Draw "1"
                glVertex2f(charX + charWidth * 0.4f, charY);
                glVertex2f(charX + charWidth * 0.6f, charY);
                glVertex2f(charX + charWidth * 0.6f, charY + charHeight);
                glVertex2f(charX + charWidth * 0.4f, charY + charHeight);
            }
            else
            {
                // Draw "0"
                const int SEGMENTS = 8;
                float centerX = charX + charWidth * 0.5f;
                float centerY = charY + charHeight * 0.5f;
                float radiusX = charWidth * 0.3f;
                float radiusY = charHeight * 0.4f;

                glBegin(GL_LINE_LOOP);
                for (int i = 0; i < SEGMENTS; i++)
                {
                    float angle = 2.0f * M_PI * i / SEGMENTS;
                    float x = centerX + cos(angle) * radiusX;
                    float y = centerY + sin(angle) * radiusY;
                    glVertex2f(x, y);
                }
                glEnd();
            }
            glEnd();
        }
    }
}