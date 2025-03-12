#include "multi_band_waveform.h"
#include <algorithm>
#include <cmath>

// Define the static constexpr color arrays
constexpr float MultiBandWaveform::LOW_COLOR[3];
constexpr float MultiBandWaveform::MID_COLOR[3];
constexpr float MultiBandWaveform::HIGH_COLOR[3];

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

    // Apply Hanning window and fill FFT input buffer
    for (int i = 0; i < N; i++)
    {
        if (sampleIndex + i < audioData.size())
        {
            // Apply Hanning window to reduce spectral leakage
            double window = 0.5 * (1.0 - cos(2.0 * M_PI * i / (N - 1)));
            fftInputBuffer[i] = audioData[sampleIndex + i] * window;
        }
        else
        {
            fftInputBuffer[i] = 0.0;
        }
    }

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

    // Apply band-specific scaling factors
    float lowScale = 2.0f;  // Boost low frequencies
    float midScale = 1.5f;  // Moderate boost for mids
    float highScale = 1.0f; // Normal scale for highs

    // Render bands in their respective positions with bipolar display
    renderBand(lowBand, -0.6f, 0.3f * lowScale, LOW_COLOR);   // Bottom band
    renderBand(midBand, 0.0f, 0.3f * midScale, MID_COLOR);    // Middle band
    renderBand(highBand, 0.6f, 0.3f * highScale, HIGH_COLOR); // Top band
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

    // Apply band-specific scaling factors
    float lowScale = 2.0f;  // Boost low frequencies
    float midScale = 1.5f;  // Moderate boost for mids
    float highScale = 1.0f; // Normal scale for highs

    // Render bands in their respective positions with bipolar display
    renderBand(lowBand, -0.6f, 0.3f * lowScale, LOW_COLOR);   // Bottom band
    renderBand(midBand, 0.0f, 0.3f * midScale, MID_COLOR);    // Middle band
    renderBand(highBand, 0.6f, 0.3f * highScale, HIGH_COLOR); // Top band
}

void MultiBandWaveform::renderBand(const std::vector<float> &bandData, float yOffset, float height, const float *color)
{
    glColor3fv(color);

    // Set line thickness to 5 pixels
    glLineWidth(5.0f);

    // Draw the waveform
    glBegin(GL_LINE_STRIP);

    // Use 200 points across the screen for smooth rendering
    const int numPoints = 200;
    const float pointSpacing = 2.0f / (numPoints - 1);

    for (int i = 0; i < numPoints; i++)
    {
        float x = -1.0f + i * pointSpacing;

        // Calculate average amplitude for this segment
        float sum = 0.0f;
        int count = 0;
        int startIdx = (i * bandData.size()) / numPoints;
        int endIdx = ((i + 1) * bandData.size()) / numPoints;

        for (int j = startIdx; j < endIdx && j < static_cast<int>(bandData.size()); j++)
        {
            sum += bandData[j];
            count++;
        }

        // Calculate average and make it bipolar
        float avgAmplitude = count > 0 ? (sum / count) : 0.0f;
        float y = yOffset + (avgAmplitude * 2.0f - 1.0f) * height;

        glVertex2f(x, y);
    }
    glEnd();

    // Reset line width to default (1.0)
    glLineWidth(1.0f);
}

std::vector<float> MultiBandWaveform::filterBand(fftw_complex *fftOutput, int startBin, int endBin)
{
    // Create a fixed-size output array for consistent width display
    const int outputSize = 200; // Match the number of points we use for rendering
    std::vector<float> bandData(outputSize, 0.0f);

    if (startBin >= endBin)
        return bandData;

    // Calculate the bin width for each output point
    float binsPerPoint = static_cast<float>(endBin - startBin) / outputSize;

    // Process the frequency bins and map them to output points
    for (int outIdx = 0; outIdx < outputSize; outIdx++)
    {
        float startBinF = startBin + outIdx * binsPerPoint;
        float endBinF = startBinF + binsPerPoint;

        int binStart = static_cast<int>(startBinF);
        int binEnd = static_cast<int>(std::ceil(endBinF));

        float sum = 0.0f;
        float weight = 0.0f;

        for (int bin = binStart; bin < binEnd && bin < N / 2; bin++)
        {
            // Calculate magnitude
            float magnitude = std::sqrt(fftOutput[bin][0] * fftOutput[bin][0] +
                                        fftOutput[bin][1] * fftOutput[bin][1]);

            // Apply logarithmic scaling
            magnitude = magnitude > 0 ? std::log10(1 + magnitude) : 0;

            // Calculate the weight for this bin (handle partial bins at boundaries)
            float binWeight = 1.0f;
            if (bin == binStart)
            {
                binWeight = 1.0f - (startBinF - binStart);
            }
            if (bin == binEnd - 1)
            {
                binWeight = std::min(binWeight, endBinF - (binEnd - 1));
            }

            sum += magnitude * binWeight;
            weight += binWeight;
        }

        bandData[outIdx] = weight > 0 ? sum / weight : 0;
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