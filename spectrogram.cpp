#include "spectrogram.h"
#include <cmath>
#include <algorithm>

Spectrogram::Spectrogram()
{
    // Initialize Hanning window
    window.resize(N);
    for (int i = 0; i < N; i++)
    {
        window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (N - 1)));
    }
}

Spectrogram::~Spectrogram()
{
}

void Spectrogram::renderFrame(const std::vector<float> &audioData,
                              double *fftInputBuffer,
                              fftw_complex *fftOutputBuffer,
                              fftw_plan &fftPlan,
                              float timeSeconds)
{
    // Calculate the sample index for the current time
    size_t sampleIndex = static_cast<size_t>(timeSeconds * 44100); // Assuming 44.1kHz
    if (sampleIndex >= audioData.size())
        return;

    // Apply window function and copy to FFT input buffer
    for (int i = 0; i < N && (sampleIndex + i) < audioData.size(); i++)
    {
        fftInputBuffer[i] = audioData[sampleIndex + i] * window[i];
    }

    // Perform FFT
    fftw_execute(fftPlan);

    // Render the spectrum
    renderSpectrum(fftOutputBuffer);
}

void Spectrogram::renderLiveFrame(const std::vector<float> &audioData,
                                  double *fftInputBuffer,
                                  fftw_complex *fftOutputBuffer,
                                  fftw_plan &fftPlan,
                                  size_t currentPosition)
{
    // Apply window function and copy to FFT input buffer
    for (int i = 0; i < N && (currentPosition + i) < audioData.size(); i++)
    {
        fftInputBuffer[i] = audioData[currentPosition + i] * window[i];
    }

    // Perform FFT
    fftw_execute(fftPlan);

    // Render the spectrum
    renderSpectrum(fftOutputBuffer);
}

void Spectrogram::renderSpectrum(const fftw_complex *fftData)
{
    // We only need the first N/2 + 1 points due to symmetry
    const int numPoints = N / 2 + 1;

    // Set color gradient from blue to red
    glBegin(GL_TRIANGLE_STRIP);

    // Add bottom vertices for filled area (at y = -1)
    glColor3f(0.0f, 0.0f, 0.1f); // Dark blue at the bottom
    glVertex2f(-1.0f, -1.0f);

    // Calculate magnitudes and render spectrum
    for (int i = 0; i < numPoints; i++)
    {
        // Calculate frequency bin position
        float x = -1.0f + 2.0f * i / (float)(numPoints - 1);

        // Calculate magnitude (using log scale for better visualization)
        float re = fftData[i][0];
        float im = fftData[i][1];
        float magnitude = std::sqrt(re * re + im * im);
        float dB = 20.0f * std::log10(magnitude + 1e-6f); // Add small value to avoid log(0)

        // Normalize to [-1, 1] range
        float y = std::max(-1.0f, std::min(1.0f, dB / 60.0f)); // Assuming typical range of -60dB to 0dB

        // Calculate color based on magnitude (blue to red gradient)
        float intensity = (y + 1.0f) / 2.0f; // Map [-1,1] to [0,1]
        glColor3f(intensity, 0.2f * intensity, 1.0f - intensity);

        // Add bottom vertex for filled area
        glVertex2f(x, -1.0f);
        // Add top vertex for spectrum
        glVertex2f(x, y);
    }

    glEnd();
}