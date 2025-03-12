#include "multi_band_waveform.h"
#include <algorithm>
#include <cmath>

MultiBandWaveform::MultiBandWaveform()
{
}

MultiBandWaveform::~MultiBandWaveform()
{
}

void MultiBandWaveform::renderFrame(const std::vector<float> &audioData,
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

    // Calculate frequency bin indices for cutoff frequencies
    // freq = bin * sampleRate / N
    // bin = freq * N / sampleRate
    const int lowBin = LOW_CUTOFF * N / 44100;
    const int midBin = MID_CUTOFF * N / 44100;
    const int highBin = HIGH_CUTOFF * N / 44100;

    // Filter and render each band
    std::vector<float> lowBand = filterBand(fftOutputBuffer, 0, lowBin);
    std::vector<float> midBand = filterBand(fftOutputBuffer, lowBin, midBin);
    std::vector<float> highBand = filterBand(fftOutputBuffer, midBin, highBin);

    // Render bands in their respective positions
    renderBand(lowBand, -0.6f, 0.4f, LOW_COLOR);  // Bottom band
    renderBand(midBand, 0.0f, 0.4f, MID_COLOR);   // Middle band
    renderBand(highBand, 0.6f, 0.4f, HIGH_COLOR); // Top band
}

void MultiBandWaveform::renderLiveFrame(const std::vector<float> &audioData,
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

    // Calculate frequency bin indices for cutoff frequencies
    const int lowBin = LOW_CUTOFF * N / 44100;
    const int midBin = MID_CUTOFF * N / 44100;
    const int highBin = HIGH_CUTOFF * N / 44100;

    // Filter and render each band
    std::vector<float> lowBand = filterBand(fftOutputBuffer, 0, lowBin);
    std::vector<float> midBand = filterBand(fftOutputBuffer, lowBin, midBin);
    std::vector<float> highBand = filterBand(fftOutputBuffer, midBin, highBin);

    // Render bands in their respective positions
    renderBand(lowBand, -0.6f, 0.4f, LOW_COLOR);  // Bottom band
    renderBand(midBand, 0.0f, 0.4f, MID_COLOR);   // Middle band
    renderBand(highBand, 0.6f, 0.4f, HIGH_COLOR); // Top band
}

void MultiBandWaveform::renderBand(const std::vector<float> &bandData, float yOffset, float height, const float *color)
{
    glColor3fv(color);
    glBegin(GL_LINE_STRIP);

    for (size_t i = 0; i < bandData.size(); i++)
    {
        float x = -1.0f + 2.0f * i / (float)(bandData.size() - 1);
        float y = yOffset + bandData[i] * height;
        glVertex2f(x, y);
    }

    glEnd();
}

std::vector<float> MultiBandWaveform::filterBand(fftw_complex *fftOutput, int startBin, int endBin)
{
    std::vector<float> bandData(N);

    // Zero out frequencies outside our band
    for (int i = 0; i < N / 2; i++)
    {
        if (i >= startBin && i < endBin)
        {
            // Keep the frequencies in our band
            bandData[i] = std::sqrt(fftOutput[i][0] * fftOutput[i][0] +
                                    fftOutput[i][1] * fftOutput[i][1]);
        }
        else
        {
            bandData[i] = 0.0f;
        }
    }

    // Normalize the band data
    float maxAmp = 0.0f;
    for (float amp : bandData)
    {
        maxAmp = std::max(maxAmp, amp);
    }
    if (maxAmp > 0.0f)
    {
        for (float &amp : bandData)
        {
            amp /= maxAmp;
        }
    }

    return bandData;
}