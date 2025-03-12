#include "visualizer_factory.h"
#include "bar_equalizer.h"
#include "waveform.h"
#include "multi_band_waveform.h"
#include "ascii_bar_equalizer.h"
#include "spectrogram.h"
#include "multi_band_circle_waveform.h"
#include "terrain_visualizer_3d.h"
#include <algorithm>
#include <cctype>

std::shared_ptr<Visualizer> VisualizerFactory::createVisualizer(VisualizerType type)
{
    switch (type)
    {
    case BAR_EQUALIZER:
        return std::make_shared<BarEqualizer>();
    case WAVEFORM:
        return std::make_shared<Waveform>();
    case MULTI_BAND_WAVEFORM:
        return std::make_shared<MultiBandWaveform>();
    case ASCII_BAR_EQUALIZER:
        return std::make_shared<AsciiBarEqualizer>();
    case SPECTROGRAM:
        return std::make_shared<Spectrogram>();
    case MULTI_BAND_CIRCLE_WAVEFORM:
        return std::make_shared<MultiBandCircleWaveform>();
    case TERRAIN_VISUALIZER_3D:
        return std::make_shared<TerrainVisualizer3D>();
    default:
        // Default to bar equalizer
        return std::make_shared<BarEqualizer>();
    }
}

std::shared_ptr<Visualizer> VisualizerFactory::createVisualizer(const std::string &name)
{
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });

    if (lowerName == "bars" || lowerName == "equalizer" || lowerName == "bar_equalizer")
    {
        return createVisualizer(BAR_EQUALIZER);
    }
    else if (lowerName == "wave" || lowerName == "waveform")
    {
        return createVisualizer(WAVEFORM);
    }
    else if (lowerName == "multiband" || lowerName == "multi_band" || lowerName == "multi_band_waveform")
    {
        return createVisualizer(MULTI_BAND_WAVEFORM);
    }
    else if (lowerName == "ascii" || lowerName == "ascii_bars" || lowerName == "ascii_equalizer")
    {
        return createVisualizer(ASCII_BAR_EQUALIZER);
    }
    else if (lowerName == "spectrogram" || lowerName == "spectrum")
    {
        return createVisualizer(SPECTROGRAM);
    }
    else if (lowerName == "circle" || lowerName == "circles" || lowerName == "multi_band_circle")
    {
        return createVisualizer(MULTI_BAND_CIRCLE_WAVEFORM);
    }
    else if (lowerName == "terrain" || lowerName == "3d" || lowerName == "terrain3d" || lowerName == "3d_terrain")
    {
        return createVisualizer(TERRAIN_VISUALIZER_3D);
    }
    else
    {
        // Default to bar equalizer
        return createVisualizer(BAR_EQUALIZER);
    }
}

std::string VisualizerFactory::getVisualizerName(VisualizerType type)
{
    switch (type)
    {
    case BAR_EQUALIZER:
        return "Bar Equalizer";
    case WAVEFORM:
        return "Waveform";
    case MULTI_BAND_WAVEFORM:
        return "Multi-Band Waveform";
    case ASCII_BAR_EQUALIZER:
        return "ASCII Bar Equalizer";
    case SPECTROGRAM:
        return "Spectrogram";
    case MULTI_BAND_CIRCLE_WAVEFORM:
        return "Multi-Band Circle Waveform";
    case TERRAIN_VISUALIZER_3D:
        return "3D Terrain Visualizer";
    default:
        return "Unknown";
    }
}