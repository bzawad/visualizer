#pragma once
#include "visualizer_base.h"
#include <vector>
#include <string>
#include <deque>
#include <random>

class HackerTerminal : public Visualizer
{
public:
    HackerTerminal();
    ~HackerTerminal();

    void initialize(int width, int height) override;

    void renderFrame(const std::vector<float> &audioData,
                     double *fftInputBuffer,
                     fftw_complex *fftOutputBuffer,
                     fftw_plan &fftPlan,
                     float timeSeconds) override;

    void renderLiveFrame(const std::vector<float> &audioData,
                         double *fftInputBuffer,
                         fftw_complex *fftOutputBuffer,
                         fftw_plan &fftPlan,
                         size_t currentPosition) override;

private:
    static constexpr int MAX_LINES = 50;
    static constexpr int MAX_ALERTS = 20;
    static constexpr float SCROLL_SPEED = 2.0f;
    static constexpr float ALERT_THRESHOLD = 0.3f;

    // Terminal colors (green cyberpunk theme)
    static constexpr float TEXT_COLOR[3] = {0.0f, 1.0f, 0.0f};     // Bright green
    static constexpr float DIM_TEXT_COLOR[3] = {0.0f, 0.6f, 0.0f}; // Dim green
    static constexpr float ALERT_COLOR[3] = {1.0f, 0.0f, 0.0f};    // Red alerts
    static constexpr float WARNING_COLOR[3] = {1.0f, 1.0f, 0.0f};  // Yellow warnings
    static constexpr float HEADER_COLOR[3] = {0.0f, 0.8f, 1.0f};   // Cyan headers
    static constexpr float SUCCESS_COLOR[3] = {0.0f, 1.0f, 0.5f};  // Green success

    struct TerminalLine
    {
        std::string text;
        float color[3];
        float age;
        bool isAlert;
        float intensity;
    };

    struct SystemAlert
    {
        std::string message;
        float color[3];
        float timeRemaining;
        bool isUrgent;
    };

    struct StatusBar
    {
        std::string label;
        float value;
        float maxValue;
        float color[3];
    };

    float audioAmplitude;
    float scrollPosition;
    float alertTimer;
    float hackingProgress;

    std::deque<TerminalLine> terminalLines;
    std::deque<SystemAlert> alerts;
    std::vector<StatusBar> statusBars;
    std::mt19937 rng;

    // Code generation
    std::vector<std::string> codeTemplates;
    std::vector<std::string> hackingTerms;
    std::vector<std::string> systemMessages;
    std::vector<std::string> alertMessages;

    void initializeContent();
    void updateTerminal(float deltaTime);
    void generateCodeLine();
    void generateSystemMessage();
    void generateAlert();
    void updateStatusBars();
    void renderTerminalContent();
    void renderHeader();
    void renderAlerts();
    void renderStatusBars();
    void renderScanlines();

    float calculateAudioAmplitude(const std::vector<float> &audioData, size_t position);
    std::string getCurrentTime();
    std::string generateRandomHex(int length);
    std::string generateRandomIP();
    std::string generateRandomHash();
};