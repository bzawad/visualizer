#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fftw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <sndfile.h>
#include <portaudio.h>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstring> // For strcmp

// Window dimensions
const int WIDTH = 800, HEIGHT = 600;

// Visualization type
enum VisualizerType {
    BAR_EQUALIZER,
    WAVEFORM
};
VisualizerType currentVisualizer = BAR_EQUALIZER; // Default

// FFT Settings
const int N = 1024;         // Number of samples (must be power of 2)
const int NUM_BARS = 16;    // Number of frequency bars to display
double in[N];               // Input signal
fftw_complex out[N];        // FFT output
fftw_plan plan;             // FFTW plan

// Audio playback settings
const int SAMPLE_RATE = 44100;
const int FRAMES_PER_BUFFER = 512;
std::vector<float> audioData;
std::atomic<bool> playbackFinished(false);
std::atomic<size_t> currentPosition(0);
std::mutex audioMutex;

// Forward declarations
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
bool loadWavFile(const std::string& filename);
void renderWaveform();

// Audio callback function for PortAudio
static int paCallback(const void* inputBuffer, void* outputBuffer,
                     unsigned long framesPerBuffer,
                     const PaStreamCallbackTimeInfo* timeInfo,
                     PaStreamCallbackFlags statusFlags,
                     void* userData) {
    
    // Mark unused parameters to silence compiler warnings
    (void)inputBuffer;  // Already marked as unused
    (void)timeInfo;     // Mark as unused
    (void)statusFlags;  // Mark as unused
    (void)userData;     // Mark as unused
    
    float* out = (float*)outputBuffer;
    
    size_t position = currentPosition.load();
    
    // Copy data to FFT input buffer for visualization
    std::lock_guard<std::mutex> lock(audioMutex);
    for (unsigned long i = 0; i < framesPerBuffer; i++) {
        if (position + i < audioData.size()) {
            out[i] = audioData[position + i];
            // If we have enough samples, update FFT input (reuse older samples if needed)
            if (i < N) {
                in[i] = (double)audioData[position + i];
            }
        } else {
            out[i] = 0.0f;
            playbackFinished = true;
        }
    }
    
    // Update position
    position += framesPerBuffer;
    currentPosition.store(position);
    
    return playbackFinished ? paComplete : paContinue;
}

// OpenGL rendering function for bar visualization
void renderFFT() {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0.0f, 1.0f, 0.0f); // Green visualization
    
    // Execute FFT
    fftw_execute(plan);
    
    // Calculate frequency bands for 16 bars
    float barWidth = 2.0f / NUM_BARS; // normalized width in [-1, 1] space
    
    for (int i = 0; i < NUM_BARS; i++) {
        // Map bar index to frequency range (logarithmic scale for better visual)
        int startIdx = (int)(pow(N/2, (float)i / NUM_BARS) - 1);
        int endIdx = (int)(pow(N/2, (float)(i+1) / NUM_BARS) - 1);
        startIdx = std::max(0, startIdx);
        endIdx = std::min(N/2 - 1, endIdx);
        
        // Calculate average amplitude in this frequency range
        float sum = 0.0f;
        for (int j = startIdx; j <= endIdx; j++) {
            sum += std::sqrt(out[j][0] * out[j][0] + out[j][1] * out[j][1]);
        }
        float avg = sum / (endIdx - startIdx + 1);
        
        // Normalize and apply some scaling for better visualization
        float height = std::min(1.0f, avg / 50.0f);
        
        // Draw bar
        float xLeft = -1.0f + i * barWidth;
        float xRight = xLeft + barWidth * 0.8f; // Small gap between bars
        
        glBegin(GL_QUADS);
        glVertex2f(xLeft, -1.0f);
        glVertex2f(xRight, -1.0f);
        glVertex2f(xRight, -1.0f + height * 2); // Scale to fill height
        glVertex2f(xLeft, -1.0f + height * 2);
        glEnd();
    }
}

// OpenGL rendering function for waveform visualization
void renderWaveform() {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0.0f, 1.0f, 0.0f); // Green visualization
    
    glBegin(GL_LINE_STRIP);
    
    // Display a window of samples from the current position
    size_t position = currentPosition.load();
    int sampleCount = std::min(N, (int)audioData.size() - (int)position);
    
    for (int i = 0; i < sampleCount; i++) {
        float x = -1.0f + 2.0f * i / (float)(sampleCount - 1);
        float y = audioData[position + i] * 0.8f; // Scale to prevent clipping
        glVertex2f(x, y);
    }
    
    glEnd();
}

// Load WAV file using libsndfile
bool loadWavFile(const std::string& filename) {
    SF_INFO sfInfo;
    memset(&sfInfo, 0, sizeof(sfInfo));
    
    SNDFILE* sndFile = sf_open(filename.c_str(), SFM_READ, &sfInfo);
    if (!sndFile) {
        std::cerr << "Error opening WAV file: " << sf_strerror(sndFile) << std::endl;
        return false;
    }
    
    // Check audio format
    if (sfInfo.channels != 1) {
        std::cerr << "Only mono WAV files are supported for simplicity" << std::endl;
        sf_close(sndFile);
        return false;
    }
    
    // Resize audio data buffer
    audioData.resize(sfInfo.frames);
    
    // Read the entire file
    sf_count_t count = sf_read_float(sndFile, audioData.data(), audioData.size());
    if (count != sfInfo.frames) {
        std::cerr << "Error reading WAV file: " << sf_strerror(sndFile) << std::endl;
        sf_close(sndFile);
        return false;
    }
    
    sf_close(sndFile);
    return true;
}

// Key callback function
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Mark unused parameters to silence compiler warnings
    (void)scancode;  // Mark as unused
    (void)mods;      // Mark as unused
    
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

// Main loop
int main(int argc, char** argv) {
    std::string wavFile;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--type") == 0 && i + 1 < argc) {
            if (strcmp(argv[i + 1], "waveform") == 0) {
                currentVisualizer = WAVEFORM;
            } else if (strcmp(argv[i + 1], "bars") == 0) {
                currentVisualizer = BAR_EQUALIZER;
            } else {
                std::cerr << "Unknown visualizer type: " << argv[i + 1] << std::endl;
                std::cerr << "Supported types: bars, waveform" << std::endl;
                return -1;
            }
            i++; // Skip the next argument
        } else {
            wavFile = argv[i];
        }
    }
    
    if (wavFile.empty()) {
        std::cerr << "Usage: " << argv[0] << " [--type bars|waveform] <wav_file>" << std::endl;
        return -1;
    }
    
    // Initialize PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return -1;
    }
    
    // Load WAV file
    if (!loadWavFile(wavFile)) {
        Pa_Terminate();
        return -1;
    }
    
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        Pa_Terminate();
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Music Visualizer", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        Pa_Terminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    glewInit();

    // Set up FFTW
    plan = fftw_plan_dft_r2c_1d(N, in, out, FFTW_ESTIMATE);

    // Set OpenGL viewport
    glViewport(0, 0, WIDTH, HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    
    // Set up audio stream
    PaStream* stream;
    err = Pa_OpenDefaultStream(&stream,
                              0,          // No input channels
                              1,          // Mono output
                              paFloat32,  // 32-bit floating point output
                              SAMPLE_RATE,
                              FRAMES_PER_BUFFER,
                              paCallback,
                              NULL);
    
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        fftw_destroy_plan(plan);
        glfwDestroyWindow(window);
        glfwTerminate();
        Pa_Terminate();
        return -1;
    }
    
    // Start audio stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream);
        fftw_destroy_plan(plan);
        glfwDestroyWindow(window);
        glfwTerminate();
        Pa_Terminate();
        return -1;
    }

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Render the selected visualization
        if (currentVisualizer == BAR_EQUALIZER) {
            renderFFT();
        } else if (currentVisualizer == WAVEFORM) {
            renderWaveform();
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();
        
        // If playback finished and we want to loop, reset position
        if (playbackFinished) {
            // Uncomment to enable looping
            // currentPosition.store(0);
            // playbackFinished = false;
        }
    }

    // Clean up
    if (stream) {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
    }
    
    fftw_destroy_plan(plan);
    glfwDestroyWindow(window);
    glfwTerminate();
    Pa_Terminate();
    
    return 0;
}
