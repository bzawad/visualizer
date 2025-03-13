#include "scroller_text.h"
#include <GL/glew.h>
#include <cmath>

ScrollerText::ScrollerText() : screenWidth(800), screenHeight(600), scrollPosition(0.0f) {
}

void ScrollerText::initialize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
}

void ScrollerText::renderFrame(const std::vector<float>& audioData,
                             double* in,
                             fftw_complex* out,
                             fftw_plan& plan,
                             float timeSeconds) {
    (void)audioData;  // Mark as intentionally unused
    (void)in;         // Mark as intentionally unused
    (void)plan;       // Mark as intentionally unused
    
    // Calculate magnitudes for audio reactivity
    std::vector<float> magnitudes(N/2);
    for (int i = 0; i < N/2; i++) {
        magnitudes[i] = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]) / N;
    }
    
    render(timeSeconds, magnitudes);
}

void ScrollerText::renderLiveFrame(const std::vector<float>& audioData,
                                 double* in,
                                 fftw_complex* out,
                                 fftw_plan& plan,
                                 size_t currentPosition) {
    (void)audioData;  // Mark as intentionally unused
    (void)in;         // Mark as intentionally unused
    (void)plan;       // Mark as intentionally unused
    
    // Calculate magnitudes for audio reactivity
    std::vector<float> magnitudes(N/2);
    for (int i = 0; i < N/2; i++) {
        magnitudes[i] = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]) / N;
    }
    
    float timeSeconds = static_cast<float>(currentPosition) / 44100.0f;
    render(timeSeconds, magnitudes);
}

void ScrollerText::render(float time, const std::vector<float>& magnitudes) {
    // Update scroll position
    scrollPosition = fmod(scrollPosition + SCROLL_SPEED * 0.02f, 2.0f);
    
    // Calculate audio reactivity
    float avgMagnitude = 0.0f;
    if (!magnitudes.empty()) {
        // Use lower frequencies for bounce effect (first quarter of spectrum)
        size_t numSamples = magnitudes.size() / 4;
        for (size_t i = 0; i < numSamples; i++) {
            avgMagnitude += magnitudes[i];
        }
        avgMagnitude /= numSamples;
    }
    
    // Apply bounce effect based on audio magnitude
    float bounce = avgMagnitude * 0.4f; // Increased bounce effect
    
    // Enable blending for smooth metallic effect
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Render the text with metallic effect and bounce
    renderMetallicText("Tone Coder", time, bounce);
    
    glDisable(GL_BLEND);
}

void ScrollerText::renderMetallicText(const char* text, float time, float bounce) {
    float letterSpacing = 0.3f;
    float baseX = 1.0f - scrollPosition * 2.0f; // Start from right, move left
    
    // For each character in the text
    for (const char* c = text; *c != '\0'; c++) {
        float charX = baseX + (c - text) * letterSpacing;
        
        // Calculate y position using a more aggressive sine wave
        float wavePhase = charX * 2.0f + time * SINE_FREQUENCY;
        float letterY = sin(wavePhase) * SINE_AMPLITUDE * 0.8f;
        
        // Add individual letter bounce based on audio
        letterY += bounce * sin(charX * 3.0f);
        
        // Only render if the letter is within visible range (-1.5 to 1.5)
        if (charX > -1.5f && charX < 1.5f) {
            if (*c != ' ') {  // Skip rendering for spaces but maintain spacing
                renderCharacter(*c, charX, letterY, 0.12f);
            }
        }
    }
}

void ScrollerText::renderCharacter(char c, float x, float y, float scale) {
    // Define stroke-based character shapes with adjusted positioning
    switch (c) {
        case 'T':
            renderStroke({
                {-0.35f, 1.0f}, {0.35f, 1.0f},  // Top horizontal
                {0.0f, 1.0f}, {0.0f, 0.0f}      // Vertical
            }, x, y, scale);
            break;
        case 'o':
            renderCircle(x + scale * 0.0f, y + scale * 0.5f, scale * 0.3f);
            break;
        case 'n':
            renderStroke({
                {-0.3f, 0.0f}, {-0.3f, 1.0f},    // Left vertical
                {-0.3f, 1.0f}, {0.3f, 0.0f},     // Diagonal
                {0.3f, 0.0f}, {0.3f, 1.0f}       // Right vertical
            }, x, y, scale);
            break;
        case 'e':
            renderStroke({
                {-0.3f, 0.0f}, {-0.3f, 1.0f},    // Left vertical
                {-0.3f, 0.0f}, {0.25f, 0.0f},    // Bottom horizontal
                {-0.3f, 0.5f}, {0.2f, 0.5f},     // Middle horizontal (slightly shorter)
                {-0.3f, 1.0f}, {0.25f, 1.0f}     // Top horizontal
            }, x, y, scale);
            break;
        case 'C':
            renderArc(x + scale * 0.0f, y + scale * 0.5f, scale * 0.4f, M_PI * 0.25f, M_PI * 1.75f);
            break;
        case 'd':
            renderStroke({
                {0.3f, 0.0f}, {0.3f, 1.0f}      // Right vertical
            }, x, y, scale);
            renderArc(x + scale * 0.0f, y + scale * 0.5f, scale * 0.3f, 0, M_PI * 2);
            break;
        case 'r':
            renderStroke({
                {-0.3f, 0.0f}, {-0.3f, 1.0f},    // Left vertical
                {-0.3f, 1.0f}, {0.25f, 1.0f},    // Top horizontal
                {0.25f, 1.0f}, {0.25f, 0.5f},    // Right vertical
                {0.25f, 0.5f}, {-0.3f, 0.5f},    // Middle horizontal
                {-0.3f, 0.5f}, {0.25f, 0.0f}     // Diagonal
            }, x, y, scale);
            break;
        default:
            // For unsupported characters, draw a simple rectangle
            glBegin(GL_LINE_LOOP);
            glVertex2f(x - scale * 0.4f, y);
            glVertex2f(x + scale * 0.4f, y);
            glVertex2f(x + scale * 0.4f, y + scale);
            glVertex2f(x - scale * 0.4f, y + scale);
            glEnd();
    }
}

void ScrollerText::renderStroke(const std::vector<std::pair<float, float>>& points, float x, float y, float scale) {
    glLineWidth(3.0f);  // Thicker lines for better visibility
    
    // Apply metallic gradient
    for (int i = 0; i < 4; i++) {
        glColor3fv(METALLIC_GRADIENT[i]);
        
        glBegin(GL_LINE_STRIP);
        for (size_t j = 0; j < points.size(); j += 2) {
            float x1 = x + points[j].first * scale;
            float y1 = y + points[j].second * scale;
            float x2 = x + points[j+1].first * scale;
            float y2 = y + points[j+1].second * scale;
            
            // Offset each layer slightly upward and to the right for metallic effect
            float offset = i * scale * 0.05f;  // Reduced from 0.25f to 0.05f
            glVertex2f(x1 + offset, y1 + offset);
            glVertex2f(x2 + offset, y2 + offset);
        }
        glEnd();
    }
    
    glLineWidth(1.0f);  // Reset line width
}

void ScrollerText::renderCircle(float centerX, float centerY, float radius) {
    glLineWidth(3.0f);
    
    const int segments = 32;
    const float angleStep = 2.0f * M_PI / segments;
    
    // Draw multiple circles with metallic gradient
    for (int i = 0; i < 4; i++) {
        glColor3fv(METALLIC_GRADIENT[i]);
        float r = radius * (1.0f + i * 0.1f);  // Slightly larger radius for each layer
        
        glBegin(GL_LINE_LOOP);
        for (int j = 0; j < segments; j++) {
            float angle = j * angleStep;
            float x = centerX + cos(angle) * r;
            float y = centerY + sin(angle) * r;
            glVertex2f(x, y);
        }
        glEnd();
    }
    
    glLineWidth(1.0f);
}

void ScrollerText::renderArc(float centerX, float centerY, float radius, float startAngle, float endAngle) {
    glLineWidth(3.0f);
    
    const int segments = 32;
    const float angleStep = (endAngle - startAngle) / segments;
    
    // Draw multiple arcs with metallic gradient
    for (int i = 0; i < 4; i++) {
        glColor3fv(METALLIC_GRADIENT[i]);
        float r = radius * (1.0f + i * 0.1f);  // Slightly larger radius for each layer
        
        glBegin(GL_LINE_STRIP);
        for (int j = 0; j <= segments; j++) {
            float angle = startAngle + j * angleStep;
            float x = centerX + cos(angle) * r;
            float y = centerY + sin(angle) * r;
            glVertex2f(x, y);
        }
        glEnd();
    }
    
    glLineWidth(1.0f);
} 