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
    
    // Set up perspective projection (FOV: 45 degrees)
    gluPerspective(45.0f, aspect, 0.1f, 100.0f);
    
    // Set up modelview matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Position the "camera" to look at the terrain from a good viewing angle
    // Eye position higher and slightly behind, looking down on the terrain
    gluLookAt(0.0f, 2.5f, 2.0f,  // Eye position
              0.0f, 0.0f, -2.0f, // Look-at position
              0.0f, 1.0f, 0.0f); // Up vector
              
    // Add a slight rotation for a better view angle
    glRotatef(20.0f, 1.0f, 0.0f, 0.0f);
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
    
    // Get bin ranges for each band
    const int lowStartBin = 0;
    const int lowEndBin = static_cast<int>(LOW_CUTOFF / freqResolution);
    const int midStartBin = lowEndBin;
    const int midEndBin = static_cast<int>(MID_CUTOFF / freqResolution);
    const int highStartBin = midEndBin;
    const int highEndBin = static_cast<int>(HIGH_CUTOFF / freqResolution);
    
    // Ensure bins are within the FFT range
    const int maxBin = 512; // N/2 for real signals
    
    // Bin ranges for each band
    const std::array<int, 3> startBins = {
        std::max(0, lowStartBin),
        std::max(0, midStartBin),
        std::max(0, highStartBin)
    };
    
    const std::array<int, 3> endBins = {
        std::min(maxBin, lowEndBin),
        std::min(maxBin, midEndBin),
        std::min(maxBin, highEndBin)
    };
    
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
            rawMagnitudes.push_back(magnitude);
        }
        
        // Find maximum for normalization
        float maxMagnitude = 0.0f;
        for (float mag : rawMagnitudes) {
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
    // Position bands with low frequencies in back, high frequencies in front
    const float zPositions[NUM_BANDS] = {
        -2.0f,  // Low
        -1.0f,  // Mid
        0.0f    // High
    };
    
    // Draw reference grid
    glColor3fv(GRID_COLOR);
    glLineWidth(1.0f);
    
    // Horizontal grid lines at ground level
    glBegin(GL_LINES);
    for (int z = 0; z < NUM_BANDS; z++) {
        glVertex3f(-TERRAIN_WIDTH/2, 0.0f, zPositions[z]);
        glVertex3f(TERRAIN_WIDTH/2, 0.0f, zPositions[z]);
    }
    
    // Side grid lines
    for (int x = 0; x <= 10; x++) {
        float xPos = -TERRAIN_WIDTH/2 + x * (TERRAIN_WIDTH / 10);
        glVertex3f(xPos, 0.0f, zPositions[0]);  // Back line
        glVertex3f(xPos, 0.0f, zPositions[NUM_BANDS-1]); // Front line
    }
    glEnd();
    
    // Set thick lines for the waveforms
    glLineWidth(LINE_WIDTH);
    glColor3fv(BAND_COLOR);
    
    // Draw each band's waveform
    for (int band = 0; band < NUM_BANDS; band++) {
        float z = zPositions[band];
        
        // Draw the waveform line
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i < POINTS_PER_BAND; i++) {
            float x = -TERRAIN_WIDTH/2 + i * (TERRAIN_WIDTH / (POINTS_PER_BAND - 1));
            float y = bandData[band][i] * TERRAIN_HEIGHT;
            glVertex3f(x, y, z);
        }
        glEnd();
        
        // If not the last band, draw connection lines to the next band
        if (band < NUM_BANDS - 1) {
            float nextZ = zPositions[band + 1];
            
            // Draw connection lines every 20 points
            glBegin(GL_LINES);
            for (int i = 0; i < POINTS_PER_BAND; i += 20) {
                float x = -TERRAIN_WIDTH/2 + i * (TERRAIN_WIDTH / (POINTS_PER_BAND - 1));
                float y1 = bandData[band][i] * TERRAIN_HEIGHT;
                float y2 = bandData[band + 1][i] * TERRAIN_HEIGHT;
                
                // Draw line connecting the two bands
                glVertex3f(x, y1, z);
                glVertex3f(x, y2, nextZ);
            }
            glEnd();
        }
    }
    
    // Draw the terrain mesh between bands
    for (int band = 0; band < NUM_BANDS - 1; band++) {
        float z1 = zPositions[band];
        float z2 = zPositions[band + 1];
        
        // Draw triangles to fill the space between bands
        glBegin(GL_TRIANGLE_STRIP);
        for (int i = 0; i < POINTS_PER_BAND; i += 2) { // Skip some points for less density
            float x = -TERRAIN_WIDTH/2 + i * (TERRAIN_WIDTH / (POINTS_PER_BAND - 1));
            float y1 = bandData[band][i] * TERRAIN_HEIGHT;
            float y2 = bandData[band + 1][i] * TERRAIN_HEIGHT;
            
            // First set the color to a darker green for the mesh
            glColor3f(0.0f, 0.3f, 0.1f);
            
            // Draw two vertices - one from each band
            glVertex3f(x, y1, z1);
            glVertex3f(x, y2, z2);
        }
        glEnd();
    }
    
    // Reset line width to default
    glLineWidth(1.0f);
}

#ifdef __APPLE__
#pragma clang diagnostic pop // Restore deprecation warnings
#endif 