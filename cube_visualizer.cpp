#include "cube_visualizer.h"
#include <GL/glew.h>
#include <cmath>

CubeVisualizer::CubeVisualizer() : aspectRatio(1.0f) {
}

void CubeVisualizer::initialize(int width, int height) {
    Visualizer::initialize(width, height);
    aspectRatio = static_cast<float>(width) / height;
    
    // Enable depth testing for 3D rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

void CubeVisualizer::renderFrame(const std::vector<float>& audioData,
                               double* in,
                               fftw_complex* out,
                               fftw_plan& plan,
                               float timeSeconds) {
    // Process audio data for FFT
    size_t numSamples = std::min(static_cast<size_t>(N), audioData.size());
    for (size_t i = 0; i < numSamples; i++) {
        // Apply Hanning window
        double multiplier = 0.5 * (1 - cos(2 * M_PI * i / (N - 1)));
        in[i] = audioData[i] * multiplier;
    }
    
    // Execute FFT
    fftw_execute(plan);
    
    std::vector<float> magnitudes = calculateMagnitudes(out);
    render(timeSeconds, magnitudes);
}

void CubeVisualizer::renderLiveFrame(const std::vector<float>& audioData,
                                   double* in,
                                   fftw_complex* out,
                                   fftw_plan& plan,
                                   size_t currentPosition) {
    // Process audio data for FFT
    size_t numSamples = std::min(static_cast<size_t>(N), audioData.size());
    for (size_t i = 0; i < numSamples; i++) {
        size_t index = (currentPosition + i) % audioData.size();
        // Apply Hanning window
        double multiplier = 0.5 * (1 - cos(2 * M_PI * i / (N - 1)));
        in[i] = audioData[index] * multiplier;
    }
    
    // Execute FFT
    fftw_execute(plan);
    
    float timeSeconds = static_cast<float>(currentPosition) / 44100.0f;
    std::vector<float> magnitudes = calculateMagnitudes(out);
    render(timeSeconds, magnitudes);
}

std::vector<float> CubeVisualizer::calculateMagnitudes(fftw_complex* out) {
    std::vector<float> magnitudes(N/2);
    for (size_t i = 0; i < N/2; i++) {
        magnitudes[i] = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]) / N;
    }
    return magnitudes;
}

void CubeVisualizer::render(float time, const std::vector<float>& magnitudes) {
    // Clear both color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Calculate pitch-based rotation speed (using higher frequencies for faster response)
    float pitchMagnitude = 0.0f;
    size_t pitchEndBin = std::min(static_cast<size_t>(PITCH_END_BIN), magnitudes.size());
    
    for (size_t i = PITCH_START_BIN; i < pitchEndBin; i++) {
        pitchMagnitude += magnitudes[i] * (i - PITCH_START_BIN + 1); // Weight higher frequencies more
    }
    pitchMagnitude /= (PITCH_END_BIN - PITCH_START_BIN);
    float rotationSpeed = BASE_ROTATION_SPEED + pitchMagnitude * (MAX_ROTATION_SPEED - BASE_ROTATION_SPEED);
    
    // Calculate amplitude-based bounce (using lower frequencies for punch)
    float currentAmplitude = 0.0f;
    float peakMagnitude = 0.0f;
    
    // First pass: find peak magnitude for normalization
    size_t ampEndBin = std::min(static_cast<size_t>(AMPLITUDE_END_BIN), magnitudes.size());
    for (size_t i = AMPLITUDE_START_BIN; i < ampEndBin; i++) {
        peakMagnitude = std::max(peakMagnitude, magnitudes[i]);
    }
    
    // Second pass: calculate weighted average with emphasis on peaks
    if (peakMagnitude > 0.0f) {
        for (size_t i = AMPLITUDE_START_BIN; i < ampEndBin; i++) {
            float normalizedMag = magnitudes[i] / peakMagnitude;
            currentAmplitude += normalizedMag * normalizedMag * 2.0f; // Reduced from 4.0f for smoother response
        }
        currentAmplitude /= (AMPLITUDE_END_BIN - AMPLITUDE_START_BIN);
    }
    
    // Apply smoothing between current and last amplitude
    float smoothedAmplitude = lastAmplitude + SMOOTHING_FACTOR * (currentAmplitude - lastAmplitude);
    lastAmplitude = smoothedAmplitude;  // Store for next frame
    
    float scale = BASE_SCALE + smoothedAmplitude * BOUNCE_FACTOR;
    
    // Set up perspective projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // Modern perspective projection
    float fovy = 45.0f * M_PI / 180.0f; // Convert to radians
    float f = 1.0f / tan(fovy / 2.0f);
    float zNear = 0.1f;
    float zFar = 100.0f;
    
    float projMatrix[16] = {
        f/aspectRatio, 0.0f, 0.0f, 0.0f,
        0.0f, f, 0.0f, 0.0f,
        0.0f, 0.0f, (zFar+zNear)/(zNear-zFar), -1.0f,
        0.0f, 0.0f, (2.0f*zFar*zNear)/(zNear-zFar), 0.0f
    };
    
    glLoadMatrixf(projMatrix);
    
    // Set up modelview matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, -0.40f, -4.0f);  // Move to the left (changed from 0.25f to -0.25f)
    
    // Add a slight tilt for better perspective
    glRotatef(20.0f, 1.0f, 0.0f, 0.0f);
    
    // Draw the cube
    drawCube(time * rotationSpeed, scale);
}

void CubeVisualizer::drawCube(float rotationAngle, float scale) {
    glLineWidth(LINE_WIDTH);
    
    // Set up the cube's position and rotation
    glPushMatrix();
    
    // Center the cube in world space
    glTranslatef(0.0f, 0.5f, 0.0f);  // Move up slightly to center vertically
    
    // Base rotation around Y axis
    glRotatef(rotationAngle * 2.0f, 0.0f, 1.0f, 0.0f);  // Doubled Y-axis rotation speed
    
    // Additional rotations for more dynamic motion
    glRotatef(rotationAngle * 0.7f + 30.0f, 1.0f, 0.0f, 0.0f);  // X-axis rotation with offset
    glRotatef(rotationAngle * 0.5f, 0.0f, 0.0f, 1.0f);  // Z-axis rotation
    
    // Scale uniformly
    glScalef(scale, scale, scale);
    
    // Draw the cube edges in pure white
    glBegin(GL_LINES);
    for (size_t i = 0; i < edges.size(); i += 2) {
        // Set color to pure white
        glColor3f(1.0f, 1.0f, 1.0f);
        
        // Get vertex indices for this edge
        int v1 = edges[i];
        int v2 = edges[i + 1];
        
        // Draw the edge
        glVertex3f(vertices[v1 * 3], vertices[v1 * 3 + 1], vertices[v1 * 3 + 2]);
        glVertex3f(vertices[v2 * 3], vertices[v2 * 3 + 1], vertices[v2 * 3 + 2]);
    }
    glEnd();
    
    glPopMatrix();
    glLineWidth(1.0f);  // Reset line width
} 