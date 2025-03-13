#pragma once

#include "visualizer_base.h"
#include <string>
#include <vector>
#include <utility>

class ScrollerText : public Visualizer {
public:
    ScrollerText();
    ~ScrollerText() = default;

    void initialize(int width, int height) override;
    
    // Implement base class methods
    void renderFrame(const std::vector<float>& audioData,
                    double* in,
                    fftw_complex* out,
                    fftw_plan& plan,
                    float timeSeconds) override;
                    
    void renderLiveFrame(const std::vector<float>& audioData,
                        double* in,
                        fftw_complex* out,
                        fftw_plan& plan,
                        size_t currentPosition) override;
    
    // Original scroller methods
    void render(float time, const std::vector<float>& magnitudes);
    
private:
    void renderMetallicText(const char* text, float time, float bounce);
    void renderCharacter(char c, float x, float y, float scale);
    void renderStroke(const std::vector<std::pair<float, float>>& points, float x, float y, float scale);
    void renderCircle(float centerX, float centerY, float radius);
    void renderArc(float centerX, float centerY, float radius, float startAngle, float endAngle);
    
    int screenWidth;
    int screenHeight;
    float scrollPosition;
    static constexpr float SCROLL_SPEED = 0.5f;
    static constexpr float SINE_AMPLITUDE = 0.5f;
    static constexpr float SINE_FREQUENCY = 3.0f;
    static constexpr float METALLIC_GRADIENT[5][3] = {
        {0.8f, 0.8f, 0.9f},  // Light metallic
        {0.6f, 0.6f, 0.7f},  // Mid metallic
        {0.4f, 0.4f, 0.5f},  // Dark metallic
        {0.6f, 0.6f, 0.7f},  // Mid metallic
        {0.8f, 0.8f, 0.9f}   // Light metallic
    };
}; 