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
#include <memory>

// Include our visualization components
#include "visualizer_base.h"
#include "visualizer_factory.h"
#include "waveform.h"  // Add this include for Waveform class
#include "multi_band_waveform.h"  // Add this include for MultiBandWaveform class
#include "multi_band_circle_waveform.h"  // Add this include for MultiBandCircleWaveform class
#include "grid_visualizer.h"

// FFmpeg libraries
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>
}

// Window dimensions
const int WIDTH = 800, HEIGHT = 600;

// Current visualizer type
VisualizerType currentVisualizerType = BAR_EQUALIZER; // Default
std::shared_ptr<Visualizer> currentVisualizer;

// FFT Settings
const int N = 1024;  // Number of samples (must be power of 2)
double in[N];        // Input signal
fftw_complex out[N]; // FFT output
fftw_plan plan;      // FFTW plan

// Audio settings
const int SAMPLE_RATE = 44100;
const int FRAMES_PER_BUFFER = 512;         // For PortAudio
const int OUTPUT_CHANNELS = 1;             // Always output mono audio for live playback
std::vector<float> audioData;              // Our internal buffer is always mono for visualization
std::vector<float> originalAudioData;      // Store original multi-channel audio
int originalChannels = 1;                  // Number of channels in the original audio
std::atomic<bool> playbackFinished(false); // For live mode
std::atomic<size_t> currentPosition(0);    // For live mode
std::mutex audioMutex;                     // For live mode

// Video recording settings
bool recordVideo = false;
std::string outputVideoFile;
const int FPS = 30;
AVFormatContext *formatContext = nullptr;
AVCodecContext *videoCodecContext = nullptr;
AVCodecContext *audioCodecContext = nullptr;
AVStream *videoStream = nullptr;
AVStream *audioStream = nullptr;
SwsContext *swsContext = nullptr;
AVFrame *videoFrame = nullptr;
AVFrame *rgbFrame = nullptr;
AVFrame *audioFrame = nullptr;
AVPacket *packet = nullptr;
std::vector<uint8_t> frameBuffer;

// Add to the top of the file with other global variables
std::vector<std::vector<float>> multiAudioData;  // Store multiple audio sources
std::vector<std::string> audioFilenames;         // Store filenames for multiple sources

// Forward declarations
void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
void framebufferSizeCallback(GLFWwindow *window, int width, int height);
bool loadWavFile(const std::string &filename);
void renderFrameAtTime(float timeSeconds);
bool initializeVideoEncoder();
void finalizeVideoEncoder();
void encodeVideoFrame(int frameIndex);
void encodeAudioForFrame(int frameIndex);

// Audio callback function for PortAudio (for live playback)
static int paCallback(const void *inputBuffer, void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo *timeInfo,
                      PaStreamCallbackFlags statusFlags,
                      void *userData)
{
    // Mark unused parameters to silence compiler warnings
    (void)inputBuffer;
    (void)timeInfo;
    (void)statusFlags;
    (void)userData;

    float *out = (float *)outputBuffer;
    size_t position = currentPosition.load();
    bool allFinished = true;

    // Clear output buffer
    for (unsigned long i = 0; i < framesPerBuffer; i++) {
        out[i] = 0.0f;
    }

    // Mix all audio sources together
    std::lock_guard<std::mutex> lock(audioMutex);
    for (const auto& source : multiAudioData) {
        for (unsigned long i = 0; i < framesPerBuffer; i++) {
            if (position + i < source.size()) {
                // Mix audio with equal weighting (1/number of sources)
                out[i] += source[position + i] / static_cast<float>(multiAudioData.size());
                
                // If we have enough samples, update FFT input
                if (i < N) {
                    in[i] = static_cast<double>(out[i]);
                }
                
                allFinished = false;
            }
        }
    }

    // Update position
    position += framesPerBuffer;
    currentPosition.store(position);
    playbackFinished = allFinished;

    return playbackFinished ? paComplete : paContinue;
}

// Render a frame at the specified time
void renderFrameAtTime(float timeSeconds)
{
    // The OpenGL state (viewport, matrices) is now set by the caller
    glClear(GL_COLOR_BUFFER_BIT);

    // Use the current visualizer to render the frame with multiple audio sources
    currentVisualizer->renderFrame(multiAudioData, in, out, plan, timeSeconds);
}

// OpenGL rendering function for live mode
void renderLiveVisualization()
{
    // Apply the same consistent viewport and matrix settings
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Use the current visualizer to render the live frame with multiple audio sources
    currentVisualizer->renderLiveFrame(multiAudioData, in, out, plan, currentPosition.load());
}

// Initialize video encoder
bool initializeVideoEncoder()
{
    // Allocate frame buffer
    frameBuffer.resize(WIDTH * HEIGHT * 3); // RGB format

    // Initialize FFmpeg components
    const AVCodec *videoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!videoCodec)
    {
        std::cerr << "Could not find H.264 encoder" << std::endl;
        return false;
    }

    const AVCodec *audioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!audioCodec)
    {
        std::cerr << "Could not find AAC encoder" << std::endl;
        return false;
    }

    // Create output format context
    if (avformat_alloc_output_context2(&formatContext, nullptr, nullptr, outputVideoFile.c_str()) < 0)
    {
        std::cerr << "Could not create output context" << std::endl;
        return false;
    }

    // Set up video codec context
    videoCodecContext = avcodec_alloc_context3(videoCodec);
    if (!videoCodecContext)
    {
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
    if (avcodec_open2(videoCodecContext, videoCodec, nullptr) < 0)
    {
        std::cerr << "Could not open video codec" << std::endl;
        return false;
    }

    // Add video stream
    videoStream = avformat_new_stream(formatContext, nullptr);
    if (!videoStream)
    {
        std::cerr << "Could not create video stream" << std::endl;
        return false;
    }

    videoStream->time_base = videoCodecContext->time_base;
    avcodec_parameters_from_context(videoStream->codecpar, videoCodecContext);

    // Set up audio codec context
    audioCodecContext = avcodec_alloc_context3(audioCodec);
    if (!audioCodecContext)
    {
        std::cerr << "Could not allocate audio codec context" << std::endl;
        return false;
    }

    // Set audio codec parameters
    audioCodecContext->sample_fmt = AV_SAMPLE_FMT_FLTP; // planar float format
    audioCodecContext->sample_rate = SAMPLE_RATE;
#if LIBAVUTIL_VERSION_MAJOR >= 57
    audioCodecContext->ch_layout = (originalChannels > 1) ? (AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO : (AVChannelLayout)AV_CHANNEL_LAYOUT_MONO;
#else
    audioCodecContext->channel_layout = (originalChannels > 1) ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO;
    audioCodecContext->channels = (originalChannels > 1) ? 2 : OUTPUT_CHANNELS;
#endif
    audioCodecContext->time_base = (AVRational){1, SAMPLE_RATE};
    audioCodecContext->bit_rate = 128000;

    // Open audio codec
    if (avcodec_open2(audioCodecContext, audioCodec, nullptr) < 0)
    {
        std::cerr << "Could not open audio codec" << std::endl;
        return false;
    }

    // Add audio stream
    audioStream = avformat_new_stream(formatContext, nullptr);
    if (!audioStream)
    {
        std::cerr << "Could not create audio stream" << std::endl;
        return false;
    }

    audioStream->time_base = audioCodecContext->time_base;
    avcodec_parameters_from_context(audioStream->codecpar, audioCodecContext);

    // Open output file
    if (!(formatContext->oformat->flags & AVFMT_NOFILE))
    {
        if (avio_open(&formatContext->pb, outputVideoFile.c_str(), AVIO_FLAG_WRITE) < 0)
        {
            std::cerr << "Could not open output file: " << outputVideoFile << std::endl;
            return false;
        }
    }

    // Write file header
    if (avformat_write_header(formatContext, nullptr) < 0)
    {
        std::cerr << "Could not write header" << std::endl;
        return false;
    }

    // Allocate video frames
    videoFrame = av_frame_alloc();
    rgbFrame = av_frame_alloc();
    if (!videoFrame || !rgbFrame)
    {
        std::cerr << "Could not allocate video frames" << std::endl;
        return false;
    }

    videoFrame->format = videoCodecContext->pix_fmt;
    videoFrame->width = videoCodecContext->width;
    videoFrame->height = videoCodecContext->height;

    rgbFrame->format = AV_PIX_FMT_RGB24;
    rgbFrame->width = videoCodecContext->width;
    rgbFrame->height = videoCodecContext->height;

    if (av_frame_get_buffer(videoFrame, 0) < 0 || av_frame_get_buffer(rgbFrame, 0) < 0)
    {
        std::cerr << "Could not allocate frame buffers" << std::endl;
        return false;
    }

    // Allocate audio frame - we'll use the frame size reported by the encoder
    int frameSize = audioCodecContext->frame_size;
    if (frameSize <= 0)
    {
        // AAC typically uses 1024 samples per frame
        frameSize = 1024;
        std::cout << "Using default AAC frame size: " << frameSize << std::endl;
    }
    else
    {
        std::cout << "AAC encoder frame size: " << frameSize << std::endl;
    }

    audioFrame = av_frame_alloc();
    if (!audioFrame)
    {
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

    if (av_frame_get_buffer(audioFrame, 0) < 0)
    {
        std::cerr << "Could not allocate audio frame buffer" << std::endl;
        return false;
    }

    // Log audio encoding information
    std::cout << "Audio codec configured: "
              << (originalChannels > 1 ? "Stereo" : "Mono")
              << " output at " << SAMPLE_RATE << " Hz" << std::endl;

    // Initialize conversion context
    swsContext = sws_getContext(
        WIDTH, HEIGHT, AV_PIX_FMT_RGB24,
        WIDTH, HEIGHT, videoCodecContext->pix_fmt,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!swsContext)
    {
        std::cerr << "Could not initialize conversion context" << std::endl;
        return false;
    }

    // Allocate packet
    packet = av_packet_alloc();
    if (!packet)
    {
        std::cerr << "Could not allocate packet" << std::endl;
        return false;
    }

    std::cout << "Video encoder initialized successfully" << std::endl;
    return true;
}

// Finalize video encoding and close file
void finalizeVideoEncoder()
{
    if (!recordVideo || !formatContext)
        return;

    // Flush video encoder
    avcodec_send_frame(videoCodecContext, nullptr);
    while (true)
    {
        int ret = avcodec_receive_packet(videoCodecContext, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;

        av_packet_rescale_ts(packet, videoCodecContext->time_base, videoStream->time_base);
        packet->stream_index = videoStream->index;
        av_interleaved_write_frame(formatContext, packet);
        av_packet_unref(packet);
    }

    // Flush audio encoder
    avcodec_send_frame(audioCodecContext, nullptr);
    while (true)
    {
        int ret = avcodec_receive_packet(audioCodecContext, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;

        av_packet_rescale_ts(packet, audioCodecContext->time_base, audioStream->time_base);
        packet->stream_index = audioStream->index;
        av_interleaved_write_frame(formatContext, packet);
        av_packet_unref(packet);
    }

    // Write file trailer
    av_write_trailer(formatContext);

    // Close file
    if (!(formatContext->oformat->flags & AVFMT_NOFILE))
    {
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
void encodeVideoFrame(int frameIndex)
{
    if (!recordVideo || !formatContext)
        return;

    // Ensure viewport and projection are set correctly before capturing frame
    glViewport(0, 0, WIDTH, HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Read pixels from OpenGL framebuffer
    glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, frameBuffer.data());

    // Fill RGB frame with pixel data (flipping vertically to correct orientation)
    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            int srcPos = ((HEIGHT - 1 - y) * WIDTH + x) * 3;
            int dstPos = (y * rgbFrame->linesize[0]) + (x * 3);

            rgbFrame->data[0][dstPos] = frameBuffer[srcPos];         // R
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
    if (avcodec_send_frame(videoCodecContext, videoFrame) < 0)
    {
        std::cerr << "Error sending frame to encoder" << std::endl;
        return;
    }

    while (true)
    {
        int ret = avcodec_receive_packet(videoCodecContext, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        if (ret < 0)
        {
            std::cerr << "Error receiving packet from encoder" << std::endl;
            break;
        }

        av_packet_rescale_ts(packet, videoCodecContext->time_base, videoStream->time_base);
        packet->stream_index = videoStream->index;

        if (av_interleaved_write_frame(formatContext, packet) < 0)
        {
            std::cerr << "Error writing frame to file" << std::endl;
        }

        av_packet_unref(packet);
    }
}

// Encode audio data corresponding to the specified frame
void encodeAudioForFrame(int frameIndex)
{
    if (!recordVideo || !formatContext)
        return;

    // Get audio frame size from the context
    const int frameSize = audioFrame->nb_samples;
    if (frameSize <= 0)
    {
        std::cerr << "Invalid audio frame size" << std::endl;
        return;
    }

    // Calculate sample rate per frame for perfect alignment
    const double samplesPerFrame = static_cast<double>(SAMPLE_RATE) / FPS;

    // Calculate start and end sample for this frame
    int64_t startSample = static_cast<int64_t>(frameIndex * samplesPerFrame);
    int64_t endSample = static_cast<int64_t>((frameIndex + 1) * samplesPerFrame);

    // Process audio in chunks of frameSize
    for (int64_t pos = startSample; pos < endSample; pos += frameSize)
    {
        // Prepare the audio frame
        av_frame_make_writable(audioFrame);

        // For stereo output, we need to handle channels differently
        const bool isStereo = originalChannels > 1;

        if (isStereo)
        {
            // For planar float format (FLTP), we need separate planes for each channel
            float *audioFrameDataLeft = (float *)audioFrame->data[0];  // Left channel
            float *audioFrameDataRight = (float *)audioFrame->data[1]; // Right channel

            // Clear the audio frame buffers
            for (int i = 0; i < frameSize; i++) {
                audioFrameDataLeft[i] = 0.0f;
                audioFrameDataRight[i] = 0.0f;
            }

            // Mix all audio sources together
            for (const auto& source : multiAudioData) {
                for (int i = 0; i < frameSize; i++)
                {
                    int64_t samplePos = pos + i;
                    if (samplePos < static_cast<int64_t>(source.size()))
                    {
                        // Mix with equal weighting (1/number of sources)
                        float sample = source[samplePos] / static_cast<float>(multiAudioData.size());
                        audioFrameDataLeft[i] += sample;
                        audioFrameDataRight[i] += sample;
                    }
                }
            }
        }
        else
        {
            // Mono output
            float *audioFrameData = (float *)audioFrame->data[0];

            // Clear the audio frame buffer
            for (int i = 0; i < frameSize; i++) {
                audioFrameData[i] = 0.0f;
            }

            // Mix all audio sources together
            for (const auto& source : multiAudioData) {
                for (int i = 0; i < frameSize; i++)
                {
                    int64_t samplePos = pos + i;
                    if (samplePos < static_cast<int64_t>(source.size()))
                    {
                        // Mix with equal weighting (1/number of sources)
                        audioFrameData[i] += source[samplePos] / static_cast<float>(multiAudioData.size());
                    }
                }
            }
        }

        // Set timestamp for this audio frame
        audioFrame->pts = pos;

        // Encode this audio frame
        int ret = avcodec_send_frame(audioCodecContext, audioFrame);
        if (ret < 0)
        {
            char errBuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errBuf, AV_ERROR_MAX_STRING_SIZE);
            std::cerr << "Error sending audio frame to encoder: " << errBuf << std::endl;
            continue;
        }

        while (true)
        {
            ret = avcodec_receive_packet(audioCodecContext, packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            if (ret < 0)
            {
                std::cerr << "Error receiving audio packet from encoder" << std::endl;
                break;
            }

            av_packet_rescale_ts(packet, audioCodecContext->time_base, audioStream->time_base);
            packet->stream_index = audioStream->index;

            ret = av_interleaved_write_frame(formatContext, packet);
            if (ret < 0)
            {
                char errBuf[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(ret, errBuf, AV_ERROR_MAX_STRING_SIZE);
                std::cerr << "Error writing audio frame to file: " << errBuf << std::endl;
            }

            av_packet_unref(packet);
        }
    }
}

// Load WAV file using libsndfile
bool loadWavFile(const std::string &filename)
{
    SF_INFO sfInfo;
    memset(&sfInfo, 0, sizeof(sfInfo));

    SNDFILE *sndFile = sf_open(filename.c_str(), SFM_READ, &sfInfo);
    if (!sndFile)
    {
        std::cerr << "Error opening WAV file: " << sf_strerror(sndFile) << std::endl;
        return false;
    }

    // Print audio file information
    std::cout << "Audio file: " << filename << std::endl;
    std::cout << "Sample rate: " << sfInfo.samplerate << " Hz" << std::endl;
    std::cout << "Channels: " << sfInfo.channels << std::endl;
    std::cout << "Frames: " << sfInfo.frames << std::endl;

    // Check sample rate compatibility
    if (sfInfo.samplerate != SAMPLE_RATE) {
        std::cerr << "Warning: Sample rate mismatch. Expected " << SAMPLE_RATE 
                  << " Hz, got " << sfInfo.samplerate << " Hz" << std::endl;
        sf_close(sndFile);
        return false;
    }

    // Store the original channel count
    originalChannels = sfInfo.channels;

    // Create a new vector for this audio file's data
    std::vector<float> newAudioData;
    
    // Store original audio data
    if (sfInfo.channels > 1) {
        // For stereo/multi-channel, store the original data as interleaved
        std::vector<float> originalData(sfInfo.frames * sfInfo.channels);
        sf_count_t count = sf_read_float(sndFile, originalData.data(), originalData.size());
        if (count != sfInfo.frames * sfInfo.channels) {
            std::cerr << "Error reading WAV file: " << sf_strerror(sndFile) << std::endl;
            sf_close(sndFile);
            return false;
        }

        // Convert multi-channel to mono by averaging all channels
        std::cout << "Converting " << sfInfo.channels << " channels to mono for visualization" << std::endl;
        newAudioData.resize(sfInfo.frames);
        for (sf_count_t i = 0; i < sfInfo.frames; i++) {
            float sum = 0.0f;
            for (int ch = 0; ch < sfInfo.channels; ch++) {
                sum += originalData[i * sfInfo.channels + ch];
            }
            newAudioData[i] = sum / sfInfo.channels;
        }
    } else {
        // Mono file - read directly
        newAudioData.resize(sfInfo.frames);
        sf_count_t count = sf_read_float(sndFile, newAudioData.data(), newAudioData.size());
        if (count != sfInfo.frames) {
            std::cerr << "Error reading WAV file: " << sf_strerror(sndFile) << std::endl;
            sf_close(sndFile);
            return false;
        }
    }

    sf_close(sndFile);

    // Store the filename and audio data
    audioFilenames.push_back(filename);
    multiAudioData.push_back(newAudioData);

    // Find the longest audio file length
    size_t maxLength = 0;
    for (const auto& source : multiAudioData) {
        maxLength = std::max(maxLength, source.size());
    }

    // Pad shorter audio files with silence to match the longest one
    if (maxLength > 0) {
        for (auto& source : multiAudioData) {
            if (source.size() < maxLength) {
                std::cout << "Padding audio file with silence to match longest file length" << std::endl;
                source.resize(maxLength, 0.0f);
            }
        }
    }

    // For backward compatibility, keep the first audio file in the original audioData vector
    if (multiAudioData.size() == 1) {
        audioData = multiAudioData[0];
        originalAudioData = multiAudioData[0];
    }

    return true;
}

// Key callback function
void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    // Mark unused parameters to silence compiler warnings
    (void)scancode; // Mark as unused
    (void)mods;     // Mark as unused

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    // Toggle visualization type
    else if (key == GLFW_KEY_V && action == GLFW_PRESS)
    {
        // Cycle through visualization types
        switch (currentVisualizerType)
        {
        case BAR_EQUALIZER:
            currentVisualizerType = WAVEFORM;
            break;
        case WAVEFORM:
            currentVisualizerType = MULTI_BAND_WAVEFORM;
            break;
        case MULTI_BAND_WAVEFORM:
            currentVisualizerType = ASCII_BAR_EQUALIZER;
            break;
        case ASCII_BAR_EQUALIZER:
            currentVisualizerType = SPECTROGRAM;
            break;
        case SPECTROGRAM:
            currentVisualizerType = MULTI_BAND_CIRCLE_WAVEFORM;
            break;
        case MULTI_BAND_CIRCLE_WAVEFORM:
            currentVisualizerType = TERRAIN_VISUALIZER_3D;
            break;
        case TERRAIN_VISUALIZER_3D:
            currentVisualizerType = GRID_VISUALIZER;
            break;
        case GRID_VISUALIZER:
            currentVisualizerType = BAR_EQUALIZER;
            break;
        }

        // Create the new visualizer
        currentVisualizer = VisualizerFactory::createVisualizer(currentVisualizerType);

        std::cout << "Switched to " << VisualizerFactory::getVisualizerName(currentVisualizerType) << " visualization" << std::endl;
    }
}

// Window resize callback to handle viewport changes
void framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    // Mark unused parameter to silence compiler warning
    (void)window;

    // If we're in recording mode, we should maintain the WIDTH x HEIGHT viewport
    // regardless of the actual window size to ensure consistent rendering
    if (recordVideo)
    {
        glViewport(0, 0, WIDTH, HEIGHT);
    }
    else
    {
        // For live playback, adapt to the actual window size
        glViewport(0, 0, width, height);
    }

    // Reset the projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);

    // Switch back to modelview matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Update the visualizer's dimensions
    if (currentVisualizer)
    {
        currentVisualizer->initialize(width, height);
    }
}

// Main function
int main(int argc, char **argv)
{
    std::string visualizerTypeName = "bars"; // Default

    // Parse command line arguments
    std::vector<std::string> wavFiles;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--type") == 0 && i + 1 < argc) {
            visualizerTypeName = argv[i + 1];
            i++; // Skip the next argument
        } else if (strcmp(argv[i], "--record") == 0 && i + 1 < argc) {
            recordVideo = true;
            outputVideoFile = argv[i + 1];
            i++; // Skip the next argument
        } else {
            // Collect all WAV files
            wavFiles.push_back(argv[i]);
        }
    }

    if (wavFiles.empty()) {
        std::cerr << "Usage: " << argv[0] << " [options] <wav_files...>\n"
                  << "Options:\n"
                  << "  --type <type>       Visualization type (default: bars)\n"
                  << "                      Available types: bars, waveform, multiband, ascii,\n"
                  << "                                      spectrogram, circle, terrain\n"
                  << "  --record <file>     Record visualization to video file\n"
                  << "\n"
                  << "For waveform visualization, you can provide up to 8 WAV files.\n"
                  << "The files will be arranged in a grid layout:\n"
                  << "  1 file:   1x1 grid\n"
                  << "  2 files:  1x2 grid\n"
                  << "  3-4 files: 2x2 grid\n"
                  << "  5-6 files: 2x3 grid\n"
                  << "  7-8 files: 2x4 grid\n"
                  << "\n"
                  << "Example:\n"
                  << "  " << argv[0] << " --type waveform song1.wav song2.wav song3.wav\n"
                  << std::endl;
        return -1;
    }

    // Create the visualizer
    currentVisualizer = VisualizerFactory::createVisualizer(visualizerTypeName);

    // Get the visualizer type from the name
    if (visualizerTypeName == "waveform") {
        currentVisualizerType = WAVEFORM;
    } else if (visualizerTypeName == "multiband" || visualizerTypeName == "multi_band") {
        currentVisualizerType = MULTI_BAND_WAVEFORM;
    } else if (visualizerTypeName == "ascii") {
        currentVisualizerType = ASCII_BAR_EQUALIZER;
    } else if (visualizerTypeName == "spectrogram" || visualizerTypeName == "spectrum") {
        currentVisualizerType = SPECTROGRAM;
    } else if (visualizerTypeName == "circle" || visualizerTypeName == "circles" || visualizerTypeName == "multi_band_circle") {
        currentVisualizerType = MULTI_BAND_CIRCLE_WAVEFORM;
    } else if (visualizerTypeName == "grid") {
        currentVisualizerType = GRID_VISUALIZER;
    } else {
        currentVisualizerType = BAR_EQUALIZER;
    }

    std::cout << "Using " << VisualizerFactory::getVisualizerName(currentVisualizerType) << " visualization" << std::endl;

    // Load all WAV files (up to 9)
    size_t maxFiles = std::min(wavFiles.size(), size_t(9));
    for (size_t i = 0; i < maxFiles; i++) {
        if (!loadWavFile(wavFiles[i])) {
            return -1;
        }
    }

    // If using waveform visualizer, set the multiple audio sources
    if (currentVisualizerType == WAVEFORM) {
        Waveform* waveformVis = dynamic_cast<Waveform*>(currentVisualizer.get());
        if (waveformVis) {
            waveformVis->setAudioSources(multiAudioData);
        }
    }
    // If using multi-band waveform visualizer, set the multiple audio sources
    else if (currentVisualizerType == MULTI_BAND_WAVEFORM) {
        MultiBandWaveform* multiBandVis = dynamic_cast<MultiBandWaveform*>(currentVisualizer.get());
        if (multiBandVis) {
            multiBandVis->setAudioSources(multiAudioData);
        }
    }
    // If using multi-band circle visualizer, set the multiple audio sources
    else if (currentVisualizerType == MULTI_BAND_CIRCLE_WAVEFORM) {
        MultiBandCircleWaveform* circleVis = dynamic_cast<MultiBandCircleWaveform*>(currentVisualizer.get());
        if (circleVis) {
            circleVis->setAudioSources(multiAudioData);
        }
    }
    // If using grid visualizer, set the multiple audio sources
    else if (currentVisualizerType == GRID_VISUALIZER) {
        GridVisualizer* gridVis = dynamic_cast<GridVisualizer*>(currentVisualizer.get());
        if (gridVis) {
            gridVis->setAudioSources(multiAudioData);
        }
    }

    // Calculate total number of frames based on audio length
    int totalFrames = static_cast<int>(std::ceil(audioData.size() / (static_cast<double>(SAMPLE_RATE) / FPS)));
    std::cout << "Audio length: " << audioData.size() / static_cast<double>(SAMPLE_RATE) << " seconds" << std::endl;
    std::cout << "Total frames to render: " << totalFrames << std::endl;

    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Set window hints for a better default configuration
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    // Make window non-resizable when in recording mode to ensure consistent rendering
    if (recordVideo)
    {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        std::cout << "Fixed window size for recording mode" << std::endl;
    }
    else
    {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    }

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, recordVideo ? "Music Visualizer (Recording)" : "Music Visualizer", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback); // Set resize callback

    // Initialize GLEW
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Set up FFTW
    plan = fftw_plan_dft_r2c_1d(N, in, out, FFTW_ESTIMATE);

    // Initialize FFT input buffer with zeros
    for (int i = 0; i < N; i++)
    {
        in[i] = 0.0;
    }

    // Execute FFT once to ensure it's properly initialized
    fftw_execute(plan);

    // Set OpenGL viewport explicitly
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    // Get the window size for comparison with framebuffer size (for HiDPI detection)
    int winWidth, winHeight;
    glfwGetWindowSize(window, &winWidth, &winHeight);

    // Check if we're on a HiDPI display
    float scaleX = (float)fbWidth / winWidth;
    float scaleY = (float)fbHeight / winHeight;

    if (scaleX > 1.0f || scaleY > 1.0f)
    {
        std::cout << "HiDPI display detected (scale: " << scaleX << "x" << scaleY << ")" << std::endl;
    }

    // For recording, always use the exact dimensions regardless of actual framebuffer
    if (recordVideo)
    {
        glViewport(0, 0, WIDTH, HEIGHT);
    }
    else
    {
        glViewport(0, 0, fbWidth, fbHeight);
    }

    // Set up projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);

    // Switch to modelview matrix and initialize it
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Set clear color
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Initialize the visualizer
    currentVisualizer->initialize(WIDTH, HEIGHT);

    // Initialize video encoder if recording
    if (recordVideo)
    {
        if (!initializeVideoEncoder())
        {
            std::cerr << "Failed to initialize video encoder" << std::endl;
            recordVideo = false;
        }
    }

    // Non-real-time rendering loop for recording
    if (recordVideo)
    {
        std::cout << "Starting non-real-time rendering..." << std::endl;

        auto startTime = std::chrono::high_resolution_clock::now();

        // Ensure the viewport and projection are set up correctly before starting
        glViewport(0, 0, WIDTH, HEIGHT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-1, 1, -1, 1, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        for (int frameIndex = 0; frameIndex < totalFrames; frameIndex++)
        {
            // Calculate time for this frame
            float timeSeconds = frameIndex / static_cast<float>(FPS);

            // Apply consistent viewport and matrix settings before each render
            glViewport(0, 0, WIDTH, HEIGHT);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(-1, 1, -1, 1, -1, 1);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

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
            if (frameIndex % 30 == 0 || frameIndex == totalFrames - 1)
            {
                float progress = 100.0f * frameIndex / totalFrames;
                std::cout << "Rendering: " << progress << "% complete ("
                          << frameIndex << "/" << totalFrames << " frames)" << std::endl;
            }

            // Check if user wants to cancel
            if (glfwWindowShouldClose(window))
            {
                std::cout << "Rendering canceled by user." << std::endl;
                break;
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        std::cout << "Rendering completed in " << duration.count() / 1000.0 << " seconds." << std::endl;

        // Finalize video encoding
        finalizeVideoEncoder();
    }
    else
    {
        // Live playback mode with PortAudio
        std::cout << "Starting live playback mode..." << std::endl;

        // Initialize PortAudio
        PaError err = Pa_Initialize();
        if (err != paNoError)
        {
            std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
            fftw_destroy_plan(plan);
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }

        // Set up audio stream
        PaStream *stream;
        err = Pa_OpenDefaultStream(&stream,
                                   0,               // No input channels
                                   OUTPUT_CHANNELS, // Always mono output for live playback
                                   paFloat32,       // 32-bit floating point output
                                   SAMPLE_RATE,
                                   FRAMES_PER_BUFFER,
                                   paCallback,
                                   NULL);

        if (err != paNoError)
        {
            std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
            Pa_Terminate();
            fftw_destroy_plan(plan);
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }

        // Reset playback flag and position
        playbackFinished = false;
        currentPosition.store(0);

        // Initialize visualization by rendering the first frame before starting audio
        renderLiveVisualization();
        glfwSwapBuffers(window);

        // Start audio stream after visualization is initialized
        err = Pa_StartStream(stream);
        if (err != paNoError)
        {
            std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
            Pa_CloseStream(stream);
            Pa_Terminate();
            fftw_destroy_plan(plan);
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }

        // Live visualization loop
        while (!glfwWindowShouldClose(window) && !playbackFinished)
        {
            // Render the visualization based on current audio position
            renderLiveVisualization();

            // Update the window
            glfwSwapBuffers(window);
            glfwPollEvents();

            // Cap the frame rate to avoid excessive CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(1000.0f / FPS)));
        }

        // Clean up PortAudio
        if (stream)
        {
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
