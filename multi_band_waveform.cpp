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

void MultiBandWaveform::setAudioSources(const std::vector<std::vector<float>>& sources) {
    audioSources = sources;
}

void MultiBandWaveform::calculateGridDimensions(int numSources, int& rows, int& cols) const {
    if (numSources <= 1) {
        rows = 1;
        cols = 1;
    } else if (numSources == 2) {
        rows = 1;
        cols = 2;
    } else if (numSources <= 4) {
        rows = 2;
        cols = 2;
    } else if (numSources <= 6) {
        rows = 2;
        cols = 3;
    } else {
        rows = 2;
        cols = 4;
    }
}

void MultiBandWaveform::processAudioForFFT(const std::vector<float>& audioData, size_t position, double* fftInputBuffer) {
    // Apply Hanning window and fill FFT input buffer
    for (int i = 0; i < N; i++) {
        if (position + i < audioData.size()) {
            // Apply Hanning window to reduce spectral leakage
            double window = 0.5 * (1.0 - cos(2.0 * M_PI * i / (N - 1)));
            fftInputBuffer[i] = audioData[position + i] * window;
        } else {
            fftInputBuffer[i] = 0.0;
        }
    }
}

void MultiBandWaveform::renderFrame(const std::vector<float> &audioData,
                                    double *fftInputBuffer,
                                    fftw_complex *fftOutputBuffer,
                                    fftw_plan &fftPlan,
                                    float timeSeconds)
{
    // Calculate the sample index for the current time
    size_t sampleIndex = static_cast<size_t>(timeSeconds * 44100); // Assuming 44.1kHz

    // Calculate grid dimensions
    int rows, cols;
    calculateGridDimensions(audioSources.empty() ? 1 : audioSources.size(), rows, cols);

    // Calculate cell dimensions
    float cellWidth = 2.0f / cols;
    float cellHeight = 2.0f / rows;

    // Process each audio source
    for (size_t sourceIdx = 0; sourceIdx < (audioSources.empty() ? 1 : audioSources.size()); sourceIdx++) {
        const auto& source = audioSources.empty() ? audioData : audioSources[sourceIdx];
        
        if (sampleIndex >= source.size())
            continue;

        // Calculate grid position
        int row = sourceIdx / cols;
        int col = sourceIdx % cols;

        // Calculate cell boundaries with padding
        float padding = 0.02f;
        float x1 = -1.0f + col * cellWidth + padding;
        float y1 = 1.0f - (row + 1) * cellHeight + padding;
        float x2 = x1 + cellWidth - 2 * padding;
        float y2 = y1 + cellHeight - 2 * padding;
        float cellCenterY = (y1 + y2) / 2.0f;
        float effectiveHeight = (y2 - y1) / 3.0f; // Divide height by 3 for the three bands

        // Process audio data for FFT
        processAudioForFFT(source, sampleIndex, fftInputBuffer);

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

        float cellWidth = x2 - x1;
        
        // Render bands in their respective positions with bipolar display
        renderBand(lowBand, cellCenterY - effectiveHeight, effectiveHeight * lowScale, x1, cellWidth, LOW_COLOR);
        renderBand(midBand, cellCenterY, effectiveHeight * midScale, x1, cellWidth, MID_COLOR);
        renderBand(highBand, cellCenterY + effectiveHeight, effectiveHeight * highScale, x1, cellWidth, HIGH_COLOR);

        // Draw cell border
        glLineWidth(1.0f);
        glColor3f(0.3f, 0.3f, 0.3f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x1 - padding, y1 - padding);
        glVertex2f(x2 + padding, y1 - padding);
        glVertex2f(x2 + padding, y2 + padding);
        glVertex2f(x1 - padding, y2 + padding);
        glEnd();
    }
}

void MultiBandWaveform::renderLiveFrame(const std::vector<float> &audioData,
                                        double *fftInputBuffer,
                                        fftw_complex *fftOutputBuffer,
                                        fftw_plan &fftPlan,
                                        size_t currentPosition)
{
    // Similar to renderFrame but using currentPosition
    // Calculate grid dimensions
    int rows, cols;
    calculateGridDimensions(audioSources.empty() ? 1 : audioSources.size(), rows, cols);

    // Calculate cell dimensions
    float cellWidth = 2.0f / cols;
    float cellHeight = 2.0f / rows;

    // Process each audio source
    for (size_t sourceIdx = 0; sourceIdx < (audioSources.empty() ? 1 : audioSources.size()); sourceIdx++) {
        const auto& source = audioSources.empty() ? audioData : audioSources[sourceIdx];
        
        if (currentPosition >= source.size())
            continue;

        // Calculate grid position
        int row = sourceIdx / cols;
        int col = sourceIdx % cols;

        // Calculate cell boundaries with padding
        float padding = 0.02f;
        float x1 = -1.0f + col * cellWidth + padding;
        float y1 = 1.0f - (row + 1) * cellHeight + padding;
        float x2 = x1 + cellWidth - 2 * padding;
        float y2 = y1 + cellHeight - 2 * padding;
        float cellCenterY = (y1 + y2) / 2.0f;
        float effectiveHeight = (y2 - y1) / 3.0f; // Divide height by 3 for the three bands

        // Process audio data for FFT
        processAudioForFFT(source, currentPosition, fftInputBuffer);

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

        float cellWidth = x2 - x1;
        
        // Render bands in their respective positions with bipolar display
        renderBand(lowBand, cellCenterY - effectiveHeight, effectiveHeight * lowScale, x1, cellWidth, LOW_COLOR);
        renderBand(midBand, cellCenterY, effectiveHeight * midScale, x1, cellWidth, MID_COLOR);
        renderBand(highBand, cellCenterY + effectiveHeight, effectiveHeight * highScale, x1, cellWidth, HIGH_COLOR);

        // Draw cell border
        glLineWidth(1.0f);
        glColor3f(0.3f, 0.3f, 0.3f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x1 - padding, y1 - padding);
        glVertex2f(x2 + padding, y1 - padding);
        glVertex2f(x2 + padding, y2 + padding);
        glVertex2f(x1 - padding, y2 + padding);
        glEnd();
    }
}

void MultiBandWaveform::renderBand(const std::vector<float> &bandData, float yOffset, float height, float xOffset, float width, const float *color)
{
    glColor3fv(color);

    // Set line thickness to 5 pixels
    glLineWidth(5.0f);

    // Draw the waveform
    glBegin(GL_LINE_STRIP);

    // Use 200 points across the screen for smooth rendering
    const int numPoints = 200;
    const float pointSpacing = width / (numPoints - 1);

    for (int i = 0; i < numPoints; i++)
    {
        float x = xOffset + i * pointSpacing;

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