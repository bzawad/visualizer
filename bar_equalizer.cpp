#include "bar_equalizer.h"
#include <cmath>
#include <algorithm>

BarEqualizer::BarEqualizer(int numBars) : numBars(numBars)
{
    // Initialize peak tracking vectors
    peakHeights.resize(numBars, 0.0f);
    peakDecay.resize(numBars, 0.0f);
}

BarEqualizer::~BarEqualizer()
{
}

void BarEqualizer::renderFrame(const std::vector<float> &audioData,
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

    // Render bars based on FFT output
    renderBars(fftOutputBuffer);
}

void BarEqualizer::renderLiveFrame(const std::vector<float> &audioData,
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

void BarEqualizer::renderBars(fftw_complex *fftOutputBuffer)
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

            // Apply gentler frequency-dependent scaling
            float freqScaling = std::pow(static_cast<float>(j) / (startIdx + 1), 0.3f);
            magnitude *= freqScaling;

            sum += magnitude;
        }
        float avg = sum / (endIdx - startIdx + 1);

        // Apply more balanced frequency band scaling
        float bandScaling = 1.0f + (static_cast<float>(i) / numBars);
        avg *= bandScaling;

        // Normalize with increased divisor for lower frequencies
        float scalingFactor;
        if (i < numBars / 3)
        {                          // Low frequencies
            scalingFactor = 50.0f; // Increased from 25.0f for low frequencies
        }
        else if (i < 2 * numBars / 3)
        { // Mid frequencies
            scalingFactor = 35.0f;
        }
        else
        { // High frequencies
            scalingFactor = 25.0f;
        }

        float height = std::min(1.0f, avg / scalingFactor);

        // Draw bar
        float xLeft = -1.0f + i * barWidth;
        float xRight = xLeft + barWidth * 0.8f; // Small gap between bars

        // Draw the main bar in green
        glColor3f(0.0f, 1.0f, 0.0f);
        glBegin(GL_QUADS);
        glVertex2f(xLeft, -1.0f);
        glVertex2f(xRight, -1.0f);
        glVertex2f(xRight, -1.0f + height * 2);
        glVertex2f(xLeft, -1.0f + height * 2);
        glEnd();

        // Update peak (using the actual height, not scaled)
        float targetPeakHeight = height; // Removed PEAK_HEIGHT scaling
        if (targetPeakHeight > peakHeights[i])
        {
            peakHeights[i] = targetPeakHeight;
            peakDecay[i] = 0.0f;
        }
        else
        {
            peakDecay[i] += PEAK_DECAY_RATE;
            peakHeights[i] = std::max(targetPeakHeight,
                                      peakHeights[i] - peakDecay[i] * peakDecay[i]);
        }

        // Draw thicker peak line in red
        glColor3f(1.0f, 0.0f, 0.0f);
        glLineWidth(3.0f); // Set line thickness to 3 pixels
        glBegin(GL_LINES);
        float peakY = -1.0f + peakHeights[i] * 2;
        glVertex2f(xLeft, peakY);
        glVertex2f(xRight, peakY);
        glEnd();
        glLineWidth(1.0f); // Reset line width to default
    }
}