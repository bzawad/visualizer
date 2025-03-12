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

// FFmpeg libraries
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

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

// Video recording settings
bool recordVideo = false;
std::string outputVideoFile;
const int FPS = 30;
AVFormatContext* formatContext = nullptr;
AVCodecContext* codecContext = nullptr;
AVStream* videoStream = nullptr;
SwsContext* swsContext = nullptr;
AVFrame* frame = nullptr;
AVFrame* rgbFrame = nullptr;
AVPacket* packet = nullptr;
int frameCount = 0;
std::vector<uint8_t> frameBuffer;

// Forward declarations
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
bool loadWavFile(const std::string& filename);
void renderWaveform();
bool initializeVideoEncoder();
void finalizeVideoEncoder();
void captureFrame();

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

// Initialize video encoder
bool initializeVideoEncoder() {
    // Allocate frame buffer
    frameBuffer.resize(WIDTH * HEIGHT * 3); // RGB format
    
    // Initialize FFmpeg components
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "Could not find H.264 encoder" << std::endl;
        return false;
    }
    
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        std::cerr << "Could not allocate video codec context" << std::endl;
        return false;
    }
    
    // Set codec parameters
    codecContext->width = WIDTH;
    codecContext->height = HEIGHT;
    codecContext->time_base = (AVRational){1, FPS};
    codecContext->framerate = (AVRational){FPS, 1};
    codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    codecContext->gop_size = 12;
    codecContext->max_b_frames = 2;
    
    // Set codec-specific options
    av_opt_set(codecContext->priv_data, "preset", "medium", 0);
    
    // Open codec
    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        std::cerr << "Could not open codec" << std::endl;
        return false;
    }
    
    // Create output format context
    if (avformat_alloc_output_context2(&formatContext, nullptr, nullptr, outputVideoFile.c_str()) < 0) {
        std::cerr << "Could not create output context" << std::endl;
        return false;
    }
    
    // Add video stream
    videoStream = avformat_new_stream(formatContext, nullptr);
    if (!videoStream) {
        std::cerr << "Could not create video stream" << std::endl;
        return false;
    }
    
    videoStream->time_base = codecContext->time_base;
    avcodec_parameters_from_context(videoStream->codecpar, codecContext);
    
    // Open output file
    if (!(formatContext->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&formatContext->pb, outputVideoFile.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Could not open output file: " << outputVideoFile << std::endl;
            return false;
        }
    }
    
    // Write file header
    if (avformat_write_header(formatContext, nullptr) < 0) {
        std::cerr << "Could not write header" << std::endl;
        return false;
    }
    
    // Allocate frames
    frame = av_frame_alloc();
    rgbFrame = av_frame_alloc();
    if (!frame || !rgbFrame) {
        std::cerr << "Could not allocate video frames" << std::endl;
        return false;
    }
    
    frame->format = codecContext->pix_fmt;
    frame->width = codecContext->width;
    frame->height = codecContext->height;
    
    rgbFrame->format = AV_PIX_FMT_RGB24;
    rgbFrame->width = codecContext->width;
    rgbFrame->height = codecContext->height;
    
    if (av_frame_get_buffer(frame, 0) < 0 || av_frame_get_buffer(rgbFrame, 0) < 0) {
        std::cerr << "Could not allocate frame buffers" << std::endl;
        return false;
    }
    
    // Initialize conversion context
    swsContext = sws_getContext(
        WIDTH, HEIGHT, AV_PIX_FMT_RGB24,
        WIDTH, HEIGHT, codecContext->pix_fmt,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );
    
    if (!swsContext) {
        std::cerr << "Could not initialize conversion context" << std::endl;
        return false;
    }
    
    // Allocate packet
    packet = av_packet_alloc();
    if (!packet) {
        std::cerr << "Could not allocate packet" << std::endl;
        return false;
    }
    
    std::cout << "Video encoder initialized successfully" << std::endl;
    return true;
}

// Finalize video encoding and close file
void finalizeVideoEncoder() {
    if (!recordVideo || !formatContext) return;
    
    // Flush encoder
    avcodec_send_frame(codecContext, nullptr);
    while (true) {
        int ret = avcodec_receive_packet(codecContext, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        
        av_packet_rescale_ts(packet, codecContext->time_base, videoStream->time_base);
        packet->stream_index = videoStream->index;
        av_interleaved_write_frame(formatContext, packet);
        av_packet_unref(packet);
    }
    
    // Write file trailer
    av_write_trailer(formatContext);
    
    // Close file
    if (!(formatContext->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&formatContext->pb);
    }
    
    // Free resources
    av_frame_free(&frame);
    av_frame_free(&rgbFrame);
    av_packet_free(&packet);
    avcodec_free_context(&codecContext);
    avformat_free_context(formatContext);
    sws_freeContext(swsContext);
    
    std::cout << "Video saved to: " << outputVideoFile << std::endl;
}

// Capture current frame for video
void captureFrame() {
    if (!recordVideo || !formatContext) return;
    
    // Read pixels from OpenGL framebuffer
    glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, frameBuffer.data());
    
    // Fill RGB frame with pixel data (flipping vertically to correct orientation)
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int srcPos = ((HEIGHT - 1 - y) * WIDTH + x) * 3;
            int dstPos = (y * rgbFrame->linesize[0]) + (x * 3);
            
            rgbFrame->data[0][dstPos]     = frameBuffer[srcPos];     // R
            rgbFrame->data[0][dstPos + 1] = frameBuffer[srcPos + 1]; // G
            rgbFrame->data[0][dstPos + 2] = frameBuffer[srcPos + 2]; // B
        }
    }
    
    // Convert RGB to YUV
    sws_scale(swsContext, rgbFrame->data, rgbFrame->linesize, 0, HEIGHT,
              frame->data, frame->linesize);
    
    // Set frame timestamp
    frame->pts = frameCount++;
    
    // Encode frame
    if (avcodec_send_frame(codecContext, frame) < 0) {
        std::cerr << "Error sending frame to encoder" << std::endl;
        return;
    }
    
    while (true) {
        int ret = avcodec_receive_packet(codecContext, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) {
            std::cerr << "Error receiving packet from encoder" << std::endl;
            break;
        }
        
        av_packet_rescale_ts(packet, codecContext->time_base, videoStream->time_base);
        packet->stream_index = videoStream->index;
        
        if (av_interleaved_write_frame(formatContext, packet) < 0) {
            std::cerr << "Error writing frame to file" << std::endl;
        }
        
        av_packet_unref(packet);
    }
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
        } else if (strcmp(argv[i], "--record") == 0 && i + 1 < argc) {
            recordVideo = true;
            outputVideoFile = argv[i + 1];
            i++; // Skip the next argument
        } else {
            wavFile = argv[i];
        }
    }
    
    if (wavFile.empty()) {
        std::cerr << "Usage: " << argv[0] << " [--type bars|waveform] [--record output.mp4] <wav_file>" << std::endl;
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
    
    // Initialize video encoder if recording
    if (recordVideo) {
        if (!initializeVideoEncoder()) {
            std::cerr << "Failed to initialize video encoder" << std::endl;
            recordVideo = false;
        }
    }
    
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
        if (recordVideo) finalizeVideoEncoder();
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
        if (recordVideo) finalizeVideoEncoder();
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
        
        // Capture frame for video if recording
        if (recordVideo) {
            captureFrame();
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();
        
        // If playback finished and we want to loop, reset position
        if (playbackFinished) {
            // If recording, finish when playback finishes
            if (recordVideo) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            
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
    
    // Finalize video if recording
    if (recordVideo) {
        finalizeVideoEncoder();
    }
    
    fftw_destroy_plan(plan);
    glfwDestroyWindow(window);
    glfwTerminate();
    Pa_Terminate();
    
    return 0;
}
