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
    (void)audioData;  // Mark as intentionally unused
    (void)in;         // Mark as intentionally unused
    (void)plan;       // Mark as intentionally unused
    
    std::vector<float> magnitudes = calculateMagnitudes(out);
    render(timeSeconds, magnitudes);
}

void CubeVisualizer::renderLiveFrame(const std::vector<float>& audioData,
                                   double* in,
                                   fftw_complex* out,
                                   fftw_plan& plan,
                                   size_t currentPosition) {
    (void)audioData;  // Mark as intentionally unused
    (void)in;         // Mark as intentionally unused
    (void)plan;       // Mark as intentionally unused
    
    float timeSeconds = static_cast<float>(currentPosition) / 44100.0f;
    std::vector<float> magnitudes = calculateMagnitudes(out);
    render(timeSeconds, magnitudes);
}

std::vector<float> CubeVisualizer::calculateMagnitudes(fftw_complex* out) {
    std::vector<float> magnitudes(N/2);
    for (int i = 0; i < N/2; i++) {
        magnitudes[i] = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]) / N;
    }
    return magnitudes;
}

void CubeVisualizer::render(float time, const std::vector<float>& magnitudes) {
    // Clear both color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Calculate pitch-based rotation speed (using higher frequencies for faster response)
    float pitchMagnitude = 0.0f;
    for (int i = PITCH_START_BIN; i < PITCH_END_BIN && i < magnitudes.size(); i++) {
        pitchMagnitude += magnitudes[i] * (i - PITCH_START_BIN + 1); // Weight higher frequencies more
    }
    pitchMagnitude /= (PITCH_END_BIN - PITCH_START_BIN);
    float rotationSpeed = BASE_ROTATION_SPEED + pitchMagnitude * (MAX_ROTATION_SPEED - BASE_ROTATION_SPEED) * 2.0f;
    
    // Calculate amplitude-based bounce (using lower frequencies for punch)
    float amplitudeMagnitude = 0.0f;
    for (int i = AMPLITUDE_START_BIN; i < AMPLITUDE_END_BIN && i < magnitudes.size(); i++) {
        amplitudeMagnitude += magnitudes[i];
    }
    amplitudeMagnitude /= (AMPLITUDE_END_BIN - AMPLITUDE_START_BIN);
    float scale = BASE_SCALE + amplitudeMagnitude * BOUNCE_FACTOR * 2.0f;
    
    // Set up perspective projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, aspectRatio, 0.1f, 100.0f);
    
    // Set up modelview matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -3.0f);  // Move back to see the cube
    
    // Add a slight tilt for better perspective
    glRotatef(30.0f, 1.0f, 0.0f, 0.0f);
    
    // Draw the cube
    drawCube(time * rotationSpeed, scale);
}

void CubeVisualizer::drawCube(float rotationAngle, float scale) {
    glLineWidth(LINE_WIDTH);
    
    // Set up the cube's position and rotation
    glPushMatrix();
    
    // Translate to make bottom corner the pivot point
    glTranslatef(-0.5f, -0.5f, -0.5f);
    
    // Rotate around the corner
    glRotatef(rotationAngle, 1.0f, 1.0f, 1.0f);
    
    // Scale from the corner pivot point
    glScalef(scale, scale, scale);
    
    // Draw the cube edges with a metallic color gradient
    glBegin(GL_LINES);
    for (size_t i = 0; i < edges.size(); i += 2) {
        // Calculate color based on position (gradient from silver to bright cyan)
        float colorPos = static_cast<float>(i) / edges.size();
        float brightness = 0.7f + colorPos * 0.3f;
        glColor3f(brightness * 0.8f, brightness * 0.9f, brightness);
        
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