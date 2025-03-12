#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fftw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <sndfile.h>
#include <portaudio.h> // Add PortAudio back for live playback
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstring> // For strcmp
#include <chrono>  // For timing

// FFmpeg libraries
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>
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

// Audio settings
const int SAMPLE_RATE = 44100;
const int FRAMES_PER_BUFFER = 512; // For PortAudio
std::vector<float> audioData;
std::atomic<bool> playbackFinished(false); // For live mode
std::atomic<size_t> currentPosition(0);    // For live mode
std::mutex audioMutex;                     // For live mode

// Video recording settings
bool recordVideo = false;
std::string outputVideoFile;
const int FPS = 30;
AVFormatContext* formatContext = nullptr;
AVCodecContext* videoCodecContext = nullptr;
AVCodecContext* audioCodecContext = nullptr;
AVStream* videoStream = nullptr;
AVStream* audioStream = nullptr;
SwsContext* swsContext = nullptr;
AVFrame* videoFrame = nullptr;
AVFrame* rgbFrame = nullptr;
AVFrame* audioFrame = nullptr;
AVPacket* packet = nullptr;
std::vector<uint8_t> frameBuffer;

// Forward declarations
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
bool loadWavFile(const std::string& filename);
void renderFrameAtTime(float timeSeconds);
bool initializeVideoEncoder();
void finalizeVideoEncoder();
void encodeVideoFrame(int frameIndex);
void encodeAudioForFrame(int frameIndex); // Remove unused parameter

// Audio callback function for PortAudio (for live playback)
static int paCallback(const void* inputBuffer, void* outputBuffer,
                     unsigned long framesPerBuffer,
                     const PaStreamCallbackTimeInfo* timeInfo,
                     PaStreamCallbackFlags statusFlags,
                     void* userData) {
    
    // Mark unused parameters to silence compiler warnings
    (void)inputBuffer;
    (void)timeInfo;
    (void)statusFlags;
    (void)userData;
    
    float* out = (float*)outputBuffer;
    
    size_t position = currentPosition.load();
    
    // Copy data to FFT input buffer for visualization
    std::lock_guard<std::mutex> lock(audioMutex);
    for (unsigned long i = 0; i < framesPerBuffer; i++) {
        if (position + i < audioData.size()) {
            out[i] = audioData[position + i];
            // If we have enough samples, update FFT input
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
void renderFFT(float timeSeconds) {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0.0f, 1.0f, 0.0f); // Green visualization
    
    // Calculate the sample index for the current time
    size_t sampleIndex = static_cast<size_t>(timeSeconds * SAMPLE_RATE);
    if (sampleIndex >= audioData.size()) return;
    
    // Fill the FFT input buffer with samples at this time
    for (int i = 0; i < N; i++) {
        if (sampleIndex + i < audioData.size()) {
            in[i] = audioData[sampleIndex + i];
        } else {
            in[i] = 0.0;
        }
    }
    
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
void renderWaveform(float timeSeconds) {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0.0f, 1.0f, 0.0f); // Green visualization
    
    // Calculate the sample index for the current time
    size_t sampleIndex = static_cast<size_t>(timeSeconds * SAMPLE_RATE);
    if (sampleIndex >= audioData.size()) return;
    
    glBegin(GL_LINE_STRIP);
    
    // Display a window of samples from the current position
    int sampleCount = std::min(N, (int)audioData.size() - (int)sampleIndex);
    
    for (int i = 0; i < sampleCount; i++) {
        float x = -1.0f + 2.0f * i / (float)(sampleCount - 1);
        float y = audioData[sampleIndex + i] * 0.8f; // Scale to prevent clipping
        glVertex2f(x, y);
    }
    
    glEnd();
}

// OpenGL rendering function for live mode (using PortAudio position)
void renderLiveVisualization() {
    if (currentVisualizer == BAR_EQUALIZER) {
        // For live mode, just execute FFT on current buffer without timeSeconds
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
    } else {
        // Waveform visualization for live mode
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
}

// Render a frame at the specified time
void renderFrameAtTime(float timeSeconds) {
    if (currentVisualizer == BAR_EQUALIZER) {
        renderFFT(timeSeconds);
    } else {
        renderWaveform(timeSeconds);
    }
}

// Initialize video encoder
bool initializeVideoEncoder() {
    // Allocate frame buffer
    frameBuffer.resize(WIDTH * HEIGHT * 3); // RGB format
    
    // Initialize FFmpeg components
    const AVCodec* videoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!videoCodec) {
        std::cerr << "Could not find H.264 encoder" << std::endl;
        return false;
    }
    
    const AVCodec* audioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!audioCodec) {
        std::cerr << "Could not find AAC encoder" << std::endl;
        return false;
    }
    
    // Create output format context
    if (avformat_alloc_output_context2(&formatContext, nullptr, nullptr, outputVideoFile.c_str()) < 0) {
        std::cerr << "Could not create output context" << std::endl;
        return false;
    }
    
    // Set up video codec context
    videoCodecContext = avcodec_alloc_context3(videoCodec);
    if (!videoCodecContext) {
        std::cerr << "Could not allocate video codec context" << std::endl;
        return false;
    }
    
    // Set video codec parameters
    videoCodecContext->width = WIDTH;
    videoCodecContext->height = HEIGHT;
    videoCodecContext->time_base = (AVRational){1, FPS};
    videoCodecContext->framerate = (AVRational){FPS, 1};
    videoCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    videoCodecContext->gop_size = 12;
    videoCodecContext->max_b_frames = 2;
    
    // Set codec-specific options
    av_opt_set(videoCodecContext->priv_data, "preset", "medium", 0);
    
    // Open video codec
    if (avcodec_open2(videoCodecContext, videoCodec, nullptr) < 0) {
        std::cerr << "Could not open video codec" << std::endl;
        return false;
    }
    
    // Add video stream
    videoStream = avformat_new_stream(formatContext, nullptr);
    if (!videoStream) {
        std::cerr << "Could not create video stream" << std::endl;
        return false;
    }
    
    videoStream->time_base = videoCodecContext->time_base;
    avcodec_parameters_from_context(videoStream->codecpar, videoCodecContext);
    
    // Set up audio codec context
    audioCodecContext = avcodec_alloc_context3(audioCodec);
    if (!audioCodecContext) {
        std::cerr << "Could not allocate audio codec context" << std::endl;
        return false;
    }
    
    // Set audio codec parameters
    audioCodecContext->sample_fmt = AV_SAMPLE_FMT_FLTP; // planar float format
    audioCodecContext->sample_rate = SAMPLE_RATE;
#if LIBAVUTIL_VERSION_MAJOR >= 57
    audioCodecContext->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_MONO;
#else
    audioCodecContext->channel_layout = AV_CH_LAYOUT_MONO;
    audioCodecContext->channels = 1;
#endif
    audioCodecContext->time_base = (AVRational){1, SAMPLE_RATE};
    audioCodecContext->bit_rate = 128000;
    
    // Open audio codec
    if (avcodec_open2(audioCodecContext, audioCodec, nullptr) < 0) {
        std::cerr << "Could not open audio codec" << std::endl;
        return false;
    }
    
    // Add audio stream
    audioStream = avformat_new_stream(formatContext, nullptr);
    if (!audioStream) {
        std::cerr << "Could not create audio stream" << std::endl;
        return false;
    }
    
    audioStream->time_base = audioCodecContext->time_base;
    avcodec_parameters_from_context(audioStream->codecpar, audioCodecContext);
    
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
    
    // Allocate video frames
    videoFrame = av_frame_alloc();
    rgbFrame = av_frame_alloc();
    if (!videoFrame || !rgbFrame) {
        std::cerr << "Could not allocate video frames" << std::endl;
        return false;
    }
    
    videoFrame->format = videoCodecContext->pix_fmt;
    videoFrame->width = videoCodecContext->width;
    videoFrame->height = videoCodecContext->height;
    
    rgbFrame->format = AV_PIX_FMT_RGB24;
    rgbFrame->width = videoCodecContext->width;
    rgbFrame->height = videoCodecContext->height;
    
    if (av_frame_get_buffer(videoFrame, 0) < 0 || av_frame_get_buffer(rgbFrame, 0) < 0) {
        std::cerr << "Could not allocate frame buffers" << std::endl;
        return false;
    }
    
    // Allocate audio frame - we'll use the frame size reported by the encoder
    int frameSize = audioCodecContext->frame_size;
    if (frameSize <= 0) {
        // AAC typically uses 1024 samples per frame
        frameSize = 1024;
        std::cout << "Using default AAC frame size: " << frameSize << std::endl;
    } else {
        std::cout << "AAC encoder frame size: " << frameSize << std::endl;
    }
    
    audioFrame = av_frame_alloc();
    if (!audioFrame) {
        std::cerr << "Could not allocate audio frame" << std::endl;
        return false;
    }
    
    audioFrame->format = audioCodecContext->sample_fmt;
#if LIBAVUTIL_VERSION_MAJOR >= 57
    audioFrame->ch_layout = audioCodecContext->ch_layout;
#else
    audioFrame->channel_layout = audioCodecContext->channel_layout;
    audioFrame->channels = audioCodecContext->channels;
#endif
    audioFrame->sample_rate = audioCodecContext->sample_rate;
    audioFrame->nb_samples = frameSize;
    
    if (av_frame_get_buffer(audioFrame, 0) < 0) {
        std::cerr << "Could not allocate audio frame buffer" << std::endl;
        return false;
    }
    
    // Initialize conversion context
    swsContext = sws_getContext(
        WIDTH, HEIGHT, AV_PIX_FMT_RGB24,
        WIDTH, HEIGHT, videoCodecContext->pix_fmt,
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
    
    // Flush video encoder
    avcodec_send_frame(videoCodecContext, nullptr);
    while (true) {
        int ret = avcodec_receive_packet(videoCodecContext, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        
        av_packet_rescale_ts(packet, videoCodecContext->time_base, videoStream->time_base);
        packet->stream_index = videoStream->index;
        av_interleaved_write_frame(formatContext, packet);
        av_packet_unref(packet);
    }
    
    // Flush audio encoder
    avcodec_send_frame(audioCodecContext, nullptr);
    while (true) {
        int ret = avcodec_receive_packet(audioCodecContext, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        
        av_packet_rescale_ts(packet, audioCodecContext->time_base, audioStream->time_base);
        packet->stream_index = audioStream->index;
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
    av_frame_free(&videoFrame);
    av_frame_free(&rgbFrame);
    av_frame_free(&audioFrame);
    av_packet_free(&packet);
    avcodec_free_context(&videoCodecContext);
    avcodec_free_context(&audioCodecContext);
    avformat_free_context(formatContext);
    sws_freeContext(swsContext);
    
    std::cout << "Video saved to: " << outputVideoFile << std::endl;
}

// Encode a video frame at the specified index
void encodeVideoFrame(int frameIndex) {
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
              videoFrame->data, videoFrame->linesize);
    
    // Set frame timestamp using frame index
    videoFrame->pts = frameIndex;
    
    // Encode frame
    if (avcodec_send_frame(videoCodecContext, videoFrame) < 0) {
        std::cerr << "Error sending frame to encoder" << std::endl;
        return;
    }
    
    while (true) {
        int ret = avcodec_receive_packet(videoCodecContext, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) {
            std::cerr << "Error receiving packet from encoder" << std::endl;
            break;
        }
        
        av_packet_rescale_ts(packet, videoCodecContext->time_base, videoStream->time_base);
        packet->stream_index = videoStream->index;
        
        if (av_interleaved_write_frame(formatContext, packet) < 0) {
            std::cerr << "Error writing frame to file" << std::endl;
        }
        
        av_packet_unref(packet);
    }
}

// Encode audio data corresponding to the specified frame
void encodeAudioForFrame(int frameIndex) {
    if (!recordVideo || !formatContext) return;
    
    // Get audio frame size from the context
    const int frameSize = audioFrame->nb_samples;
    if (frameSize <= 0) {
        std::cerr << "Invalid audio frame size" << std::endl;
        return;
    }
    
    // Calculate sample rate per frame for perfect alignment
    const double samplesPerFrame = static_cast<double>(SAMPLE_RATE) / FPS;
    
    // Calculate start and end sample for this frame
    int64_t startSample = static_cast<int64_t>(frameIndex * samplesPerFrame);
    int64_t endSample = static_cast<int64_t>((frameIndex + 1) * samplesPerFrame);
    
    // Process audio in chunks of frameSize
    for (int64_t pos = startSample; pos < endSample; pos += frameSize) {
        // Prepare the audio frame
        av_frame_make_writable(audioFrame);
        
        // For planar float format (FLTP), we need to access the first plane
        float* audioFrameData = (float*)audioFrame->data[0];
        
        // Copy samples for the current frame
        for (int i = 0; i < frameSize; i++) {
            int64_t samplePos = pos + i;
            if (samplePos < static_cast<int64_t>(audioData.size())) {
                audioFrameData[i] = audioData[samplePos];
            } else {
                audioFrameData[i] = 0.0f; // Pad with silence if needed
            }
        }
        
        // Set timestamp for this audio frame
        audioFrame->pts = pos;
        
        // Encode this audio frame
        int ret = avcodec_send_frame(audioCodecContext, audioFrame);
        if (ret < 0) {
            char errBuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errBuf, AV_ERROR_MAX_STRING_SIZE);
            std::cerr << "Error sending audio frame to encoder: " << errBuf << std::endl;
            continue;
        }
        
        while (true) {
            ret = avcodec_receive_packet(audioCodecContext, packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
            if (ret < 0) {
                std::cerr << "Error receiving audio packet from encoder" << std::endl;
                break;
            }
            
            av_packet_rescale_ts(packet, audioCodecContext->time_base, audioStream->time_base);
            packet->stream_index = audioStream->index;
            
            ret = av_interleaved_write_frame(formatContext, packet);
            if (ret < 0) {
                char errBuf[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(ret, errBuf, AV_ERROR_MAX_STRING_SIZE);
                std::cerr << "Error writing audio frame to file: " << errBuf << std::endl;
            }
            
            av_packet_unref(packet);
        }
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

// Main function
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
    
    // Load WAV file
    if (!loadWavFile(wavFile)) {
        return -1;
    }
    
    // Calculate total number of frames based on audio length
    int totalFrames = static_cast<int>(std::ceil(audioData.size() / (static_cast<double>(SAMPLE_RATE) / FPS)));
    std::cout << "Audio length: " << audioData.size() / static_cast<double>(SAMPLE_RATE) << " seconds" << std::endl;
    std::cout << "Total frames to render: " << totalFrames << std::endl;
    
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Music Visualizer", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
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
    
    // Non-real-time rendering loop for recording
    if (recordVideo) {
        std::cout << "Starting non-real-time rendering..." << std::endl;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        for (int frameIndex = 0; frameIndex < totalFrames; frameIndex++) {
            // Calculate time for this frame
            float timeSeconds = frameIndex / static_cast<float>(FPS);
            
            // Render the visualization for this time
            renderFrameAtTime(timeSeconds);
            
            // Capture and encode the video frame
            encodeVideoFrame(frameIndex);
            
            // Encode the corresponding audio segment
            encodeAudioForFrame(frameIndex);
            
            // Update the window to show progress (but don't wait for vsync)
            glfwSwapBuffers(window);
            glfwPollEvents();
            
            // Show progress
            if (frameIndex % 30 == 0 || frameIndex == totalFrames - 1) {
                float progress = 100.0f * frameIndex / totalFrames;
                std::cout << "Rendering: " << progress << "% complete (" 
                          << frameIndex << "/" << totalFrames << " frames)" << std::endl;
            }
            
            // Check if user wants to cancel
            if (glfwWindowShouldClose(window)) {
                std::cout << "Rendering canceled by user." << std::endl;
                break;
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        std::cout << "Rendering completed in " << duration.count() / 1000.0 << " seconds." << std::endl;
        
        // Finalize video encoding
        finalizeVideoEncoder();
    } else {
        // Live playback mode with PortAudio
        std::cout << "Starting live playback mode..." << std::endl;
        
        // Initialize PortAudio
        PaError err = Pa_Initialize();
        if (err != paNoError) {
            std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
            fftw_destroy_plan(plan);
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
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
            Pa_Terminate();
            fftw_destroy_plan(plan);
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }
        
        // Start audio stream
        err = Pa_StartStream(stream);
        if (err != paNoError) {
            std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
            Pa_CloseStream(stream);
            Pa_Terminate();
            fftw_destroy_plan(plan);
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }
        
        // Reset playback flag and position
        playbackFinished = false;
        currentPosition.store(0);
        
        // Live visualization loop
        while (!glfwWindowShouldClose(window) && !playbackFinished) {
            // Render the visualization based on current audio position
            renderLiveVisualization();
            
            // Update the window
            glfwSwapBuffers(window);
            glfwPollEvents();
            
            // Cap the frame rate to avoid excessive CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(1000.0f / FPS)));
        }
        
        // Clean up PortAudio
        if (stream) {
            Pa_StopStream(stream);
            Pa_CloseStream(stream);
        }
        Pa_Terminate();
    }
    
    // Clean up
    fftw_destroy_plan(plan);
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}

