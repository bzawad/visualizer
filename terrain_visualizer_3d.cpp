#include "terrain_visualizer_3d.h"
#include <algorithm>
#include <cmath>
#include <GL/glew.h>
#ifdef __APPLE__
// Silence macOS deprecation warnings for GLU functions
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

// Define static constexpr members
constexpr float TerrainVisualizer3D::BAND_COLOR[];
constexpr float TerrainVisualizer3D::GRID_COLOR[];
constexpr float TerrainVisualizer3D::LINE_WIDTH;
constexpr int TerrainVisualizer3D::POINTS_PER_BAND;

TerrainVisualizer3D::TerrainVisualizer3D()
{
    // Initialize band data buffers with zeros
    for (int band = 0; band < NUM_BANDS; band++) {
        bandData[band].resize(POINTS_PER_BAND, 0.0f);
    }
}

TerrainVisualizer3D::~TerrainVisualizer3D()
{
    // No resources to clean up
}

void TerrainVisualizer3D::initialize(int width, int height)
{
    // Call the parent's initialize method
    Visualizer::initialize(width, height);
}

void TerrainVisualizer3D::renderFrame(const std::vector<float> &audioData,
                                      double *fftInputBuffer,
                                      fftw_complex *fftOutputBuffer,
                                      fftw_plan &fftPlan,
                                      float timeSeconds)
{
    // Calculate the sample index for the current time
    size_t sampleIndex = static_cast<size_t>(timeSeconds * 44100); // Assuming 44.1kHz
    if (sampleIndex >= audioData.size())
        return;

    // Update the FFT for the current position
    updateFFT(audioData, fftInputBuffer, fftPlan, sampleIndex);
    
    // Analyze frequency bands
    analyzeBands(fftOutputBuffer);
    
    // Set up 3D perspective view
    setupPerspectiveView();
    
    // Enable line smoothing for better quality
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Enable depth testing for proper 3D rendering
    glEnable(GL_DEPTH_TEST);
    
    // Clear both color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Render the terrain visualization
    renderTerrain();
    
    // Disable the features we enabled
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}

void TerrainVisualizer3D::renderLiveFrame(const std::vector<float> &audioData,
                                         double *fftInputBuffer,
                                         fftw_complex *fftOutputBuffer,
                                         fftw_plan &fftPlan,
                                         size_t currentPosition)
{
    // Update the FFT for the current position
    updateFFT(audioData, fftInputBuffer, fftPlan, currentPosition);
    
    // Analyze frequency bands
    analyzeBands(fftOutputBuffer);
    
    // Set up 3D perspective view
    setupPerspectiveView();
    
    // Enable line smoothing for better quality
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Enable depth testing for proper 3D rendering
    glEnable(GL_DEPTH_TEST);
    
    // Clear both color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Render the terrain visualization
    renderTerrain();
    
    // Disable the features we enabled
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}

void TerrainVisualizer3D::setupPerspectiveView()
{
    // Set up projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // Calculate the aspect ratio for proper perspective
    float aspect = static_cast<float>(screenWidth) / screenHeight;
    
    // Set up perspective projection with wider FOV for more perspective effect
    gluPerspective(65.0f, aspect, 0.1f, 100.0f);
    
    // Set up modelview matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Position the "camera" to look at the terrain from a more elevated angle
    // Adjusted to make the terrain fill more vertical space
    gluLookAt(0.0f, 6.0f, 7.0f,     // Eye position (slightly further back)
              0.0f, 2.0f, -4.0f,    // Look-at position (raised more)
              0.0f, 1.0f, 0.0f);    // Up vector
              
    // Adjust rotation angle to show more vertical terrain
    glRotatef(35.0f, 1.0f, 0.0f, 0.0f);
}

void TerrainVisualizer3D::updateFFT(const std::vector<float> &audioData, double *fftInputBuffer, fftw_plan &fftPlan, size_t position)
{
    // Fill the FFT input buffer with audio data and apply a Hanning window
    const int N = 1024; // Assuming this is the FFT size
    
    for (int i = 0; i < N; i++) {
        if (position + i < audioData.size()) {
            // Apply Hanning window to reduce spectral leakage
            double window = 0.5 * (1.0 - cos(2.0 * M_PI * i / (N - 1)));
            fftInputBuffer[i] = audioData[position + i] * window;
        } else {
            fftInputBuffer[i] = 0.0;
        }
    }
    
    // Execute FFT
    fftw_execute(fftPlan);
}

void TerrainVisualizer3D::analyzeBands(fftw_complex *fftOutput)
{
    // Calculate frequency resolution
    const float freqResolution = 44100.0f / 1024.0f; // Sample rate / FFT size
    
    // Define the cutoff frequencies for the 5 bands
    const int cutoffs[NUM_BANDS + 1] = {
        0,                // Start of lowest band
        LOWEST_CUTOFF,    // End of lowest band / Start of low-mid band
        LOW_MID_CUTOFF,   // End of low-mid band / Start of mid band
        MID_CUTOFF,       // End of mid band / Start of mid-high band
        MID_HIGH_CUTOFF,  // End of mid-high band / Start of high band
        HIGH_CUTOFF       // End of high band
    };
    
    // Ensure bins are within the FFT range
    const int maxBin = 512; // N/2 for real signals
    
    // Bin ranges for each band
    std::array<int, NUM_BANDS> startBins;
    std::array<int, NUM_BANDS> endBins;
    
    for (int band = 0; band < NUM_BANDS; band++) {
        startBins[band] = std::max(0, static_cast<int>(cutoffs[band] / freqResolution));
        endBins[band] = std::min(maxBin, static_cast<int>(cutoffs[band + 1] / freqResolution));
    }
    
    // Process each band
    for (int band = 0; band < NUM_BANDS; band++) {
        // Create a temporary buffer for raw magnitudes
        std::vector<float> rawMagnitudes;
        rawMagnitudes.reserve(endBins[band] - startBins[band]);
        
        // Extract magnitudes for this band
        for (int bin = startBins[band]; bin < endBins[band]; bin++) {
            float magnitude = std::sqrt(
                fftOutput[bin][0] * fftOutput[bin][0] +
                fftOutput[bin][1] * fftOutput[bin][1]
            );
            
            // Apply logarithmic scaling for better visualization
            magnitude = magnitude > 0 ? std::log10(1 + magnitude) : 0;
            
            // Apply frequency-dependent scaling (similar to bar_equalizer.cpp)
            // Higher frequencies get more emphasis
            if (startBins[band] > 0) { // Avoid division by zero
                float freqScaling = std::pow(static_cast<float>(bin) / startBins[band], 0.5f);
                magnitude *= freqScaling;
            }
            
            rawMagnitudes.push_back(magnitude);
        }
        
        // Calculate band-specific scaling (similar to bar_equalizer.cpp)
        // This gives more emphasis to higher frequency bands
        float bandScaling = 1.0f + (static_cast<float>(band) / NUM_BANDS) * 1.5f;
        
        // Find maximum for normalization
        float maxMagnitude = 0.0f;
        for (float &mag : rawMagnitudes) {
            // Apply band scaling to each magnitude
            mag *= bandScaling;
            maxMagnitude = std::max(maxMagnitude, mag);
        }
        
        // Resample to desired output resolution and normalize
        for (int i = 0; i < POINTS_PER_BAND; i++) {
            float position = static_cast<float>(i) / POINTS_PER_BAND;
            float index = position * rawMagnitudes.size();
            
            // Linear interpolation between nearest samples
            int index1 = static_cast<int>(index);
            int index2 = std::min(index1 + 1, static_cast<int>(rawMagnitudes.size()) - 1);
            float fraction = index - index1;
            
            float value = rawMagnitudes[index1] * (1.0f - fraction) + 
                        rawMagnitudes[index2] * fraction;
            
            // Normalize and store
            if (maxMagnitude > 0.0f) {
                bandData[band][i] = value / maxMagnitude;
            } else {
                bandData[band][i] = 0.0f;
            }
        }
    }
}

void TerrainVisualizer3D::renderTerrain()
{
    // Z-spacing between bands (increased for more depth)
    const float Z_SPACING = 2.0f;
    
    // Position bands with more depth and spacing
    // Low frequencies in back, high frequencies in front
    const float zPositions[NUM_BANDS] = {
        -4.0f * Z_SPACING,  // Lowest (furthest back)
        -3.0f * Z_SPACING,  // Low-Mid
        -2.0f * Z_SPACING,  // Mid
        -1.0f * Z_SPACING,  // Mid-High
        0.0f                // Highest (front)
    };
    
    // Vary width by band - front is full width, back is 1/6 width
    // Adjusted to make more extreme differences between front and back
    const float widthScales[NUM_BANDS] = {
        0.17f,  // Lowest (narrowest)
        0.3f,   // Low-Mid
        0.5f,   // Mid
        0.75f,  // Mid-High
        1.0f    // Highest (full width)
    };
    
    // Draw reference grid
    glColor3fv(GRID_COLOR);
    glLineWidth(1.0f);
    
    // Horizontal grid lines at ground level
    glBegin(GL_LINES);
    for (int band = 0; band < NUM_BANDS; band++) {
        float z = zPositions[band];
        float width = TERRAIN_WIDTH * widthScales[band];
        glVertex3f(-width/2, 0.0f, z);
        glVertex3f(width/2, 0.0f, z);
    }
    
    // Side grid lines
    const int numGridLines = 15; // Increased for more detail with wider terrain
    for (int band = 0; band < NUM_BANDS - 1; band++) {
        float z1 = zPositions[band];
        float z2 = zPositions[band + 1];
        float width1 = TERRAIN_WIDTH * widthScales[band];
        float width2 = TERRAIN_WIDTH * widthScales[band + 1];
        
        for (int i = 0; i <= numGridLines; i++) {
            float t = static_cast<float>(i) / numGridLines;
            float x1 = -width1/2 + t * width1;
            float x2 = -width2/2 + t * width2;
            
            glVertex3f(x1, 0.0f, z1);
            glVertex3f(x2, 0.0f, z2);
        }
    }
    glEnd();
    
    // Set thick lines for the waveforms
    glLineWidth(LINE_WIDTH);
    glColor3fv(BAND_COLOR);
    
    // Draw each band's waveform with varying width
    for (int band = 0; band < NUM_BANDS; band++) {
        float z = zPositions[band];
        float width = TERRAIN_WIDTH * widthScales[band];
        
        // Draw the waveform line
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i < POINTS_PER_BAND; i++) {
            float x = -width/2 + i * (width / (POINTS_PER_BAND - 1));
            float y = bandData[band][i] * TERRAIN_HEIGHT;
            glVertex3f(x, y, z);
        }
        glEnd();
    }
    
    // Draw vertical connector lines between bands
    for (int band = 0; band < NUM_BANDS - 1; band++) {
        float z1 = zPositions[band];
        float z2 = zPositions[band + 1];
        float width1 = TERRAIN_WIDTH * widthScales[band];
        float width2 = TERRAIN_WIDTH * widthScales[band + 1];
        
        // Draw connection lines every 20 points
        glBegin(GL_LINES);
        for (int i = 0; i < POINTS_PER_BAND; i += 20) {
            float t = static_cast<float>(i) / (POINTS_PER_BAND - 1);
            float x1 = -width1/2 + t * width1;
            float x2 = -width2/2 + t * width2;
            float y1 = bandData[band][i] * TERRAIN_HEIGHT;
            float y2 = bandData[band + 1][i] * TERRAIN_HEIGHT;
            
            // Draw line connecting the two bands
            glVertex3f(x1, y1, z1);
            glVertex3f(x2, y2, z2);
        }
        glEnd();
    }
    
    // Reset line width to default
    glLineWidth(1.0f);
}

#ifdef __APPLE__
#pragma clang diagnostic pop // Restore deprecation warnings
#endif 