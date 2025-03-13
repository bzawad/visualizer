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

void MultiBandCircleWaveform::setAudioSources(const std::vector<std::vector<float>>& sources) {
    audioSources = sources;
}

void MultiBandCircleWaveform::calculateGridDimensions(int numSources, int& rows, int& cols) const {
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

void MultiBandCircleWaveform::processAudioForFFT(const std::vector<float>& audioData, size_t position, double* fftInputBuffer) {
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

void MultiBandCircleWaveform::renderFrame(const std::vector<float> &audioData,
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
        float cellCenterX = (x1 + x2) / 2.0f;
        float cellCenterY = (y1 + y2) / 2.0f;
        float scale = std::min(cellWidth, cellHeight) / 2.0f;

        // Process audio data for FFT
        processAudioForFFT(source, sampleIndex, fftInputBuffer);

        // Execute FFT
        fftw_execute(fftPlan);

        // Filter audio into frequency bands with adjusted scaling
        std::vector<float> lowBand = filterBand(fftOutputBuffer, 0, (LOW_CUTOFF * N) / 44100, 1.0f);
        std::vector<float> midBand = filterBand(fftOutputBuffer, (LOW_CUTOFF * N) / 44100, (MID_CUTOFF * N) / 44100, 2.0f);
        std::vector<float> highBand = filterBand(fftOutputBuffer, (MID_CUTOFF * N) / 44100, (HIGH_CUTOFF * N) / 44100, 3.0f);

        // Draw cell border
        glLineWidth(1.0f);
        glColor3f(0.3f, 0.3f, 0.3f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x1 - padding, y1 - padding);
        glVertex2f(x2 + padding, y1 - padding);
        glVertex2f(x2 + padding, y2 + padding);
        glVertex2f(x1 - padding, y2 + padding);
        glEnd();

        // Render each band as a circle with increased thickness
        renderCircularBand(lowBand, LOW_RADIUS, THICKNESS * 1.5f, LOW_COLOR, cellCenterX, cellCenterY, scale);
        renderCircularBand(midBand, MID_RADIUS, THICKNESS * 1.5f, MID_COLOR, cellCenterX, cellCenterY, scale);
        renderCircularBand(highBand, HIGH_RADIUS, THICKNESS * 1.5f, HIGH_COLOR, cellCenterX, cellCenterY, scale);
    }
}

void MultiBandCircleWaveform::renderLiveFrame(const std::vector<float> &audioData,
                                              double *fftInputBuffer,
                                              fftw_complex *fftOutputBuffer,
                                              fftw_plan &fftPlan,
                                              size_t currentPosition)
{
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
        float cellCenterX = (x1 + x2) / 2.0f;
        float cellCenterY = (y1 + y2) / 2.0f;
        float scale = std::min(cellWidth, cellHeight) / 2.0f;

        // Process audio data for FFT
        processAudioForFFT(source, currentPosition, fftInputBuffer);

        // Execute FFT
        fftw_execute(fftPlan);

        // Filter audio into frequency bands with adjusted scaling
        std::vector<float> lowBand = filterBand(fftOutputBuffer, 0, (LOW_CUTOFF * N) / 44100, 1.0f);
        std::vector<float> midBand = filterBand(fftOutputBuffer, (LOW_CUTOFF * N) / 44100, (MID_CUTOFF * N) / 44100, 2.0f);
        std::vector<float> highBand = filterBand(fftOutputBuffer, (MID_CUTOFF * N) / 44100, (HIGH_CUTOFF * N) / 44100, 3.0f);

        // Draw cell border
        glLineWidth(1.0f);
        glColor3f(0.3f, 0.3f, 0.3f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x1 - padding, y1 - padding);
        glVertex2f(x2 + padding, y1 - padding);
        glVertex2f(x2 + padding, y2 + padding);
        glVertex2f(x1 - padding, y2 + padding);
        glEnd();

        // Render each band as a circle with increased thickness
        renderCircularBand(lowBand, LOW_RADIUS, THICKNESS * 1.5f, LOW_COLOR, cellCenterX, cellCenterY, scale);
        renderCircularBand(midBand, MID_RADIUS, THICKNESS * 1.5f, MID_COLOR, cellCenterX, cellCenterY, scale);
        renderCircularBand(highBand, HIGH_RADIUS, THICKNESS * 1.5f, HIGH_COLOR, cellCenterX, cellCenterY, scale);
    }
}

void MultiBandCircleWaveform::renderCircularBand(const std::vector<float> &bandData, float radius, float thickness, 
                                                const float *color, float xOffset, float yOffset, float scale)
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

        // Calculate position with amplitude modulation, scaled and offset
        float r = (radius + amplitude) * scale;
        float x = xOffset + r * cos(angle);
        float y = yOffset + r * sin(angle);

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