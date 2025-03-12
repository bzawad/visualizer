#include "visualizer_factory.h"
#include "bar_equalizer.h"
#include "waveform.h"
#include "multi_band_waveform.h"
#include "ascii_bar_equalizer.h"
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
    default:
        return "Unknown";
    }
}