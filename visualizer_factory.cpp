#include "visualizer_factory.h"
#include "bar_equalizer.h"
#include "mini_bar_equalizer.h"
#include "waveform.h"
#include "multi_band_waveform.h"
#include "ascii_bar_equalizer.h"
#include "spectrogram.h"
#include "mini_spectrogram.h"
#include "mini_cube_visualizer.h"
#include "mini_circle_visualizer.h"
#include "multi_band_circle_waveform.h"
#include "terrain_visualizer_3d.h"
#include "grid_visualizer.h"
#include "scroller_text.h"
#include "cube_visualizer.h"
#include "racer_visualizer.h"
#include "mini_racer_visualizer.h"
#include "maze_visualizer.h"
#include "hacker_terminal.h"
#include "balls_visualizer.h"
#include <algorithm>
#include <cctype>

std::shared_ptr<Visualizer> VisualizerFactory::createVisualizer(VisualizerType type)
{
    switch (type)
    {
    case BAR_EQUALIZER:
        return std::make_shared<BarEqualizer>();
    case MINI_BAR_EQUALIZER:
        return std::make_shared<MiniBarEqualizer>();
    case WAVEFORM:
        return std::make_shared<Waveform>();
    case MULTI_BAND_WAVEFORM:
        return std::make_shared<MultiBandWaveform>();
    case ASCII_BAR_EQUALIZER:
        return std::make_shared<AsciiBarEqualizer>();
    case SPECTROGRAM:
        return std::make_shared<Spectrogram>();
    case MINI_SPECTROGRAM:
        return std::make_shared<MiniSpectrogram>();
    case MULTI_BAND_CIRCLE_WAVEFORM:
        return std::make_shared<MultiBandCircleWaveform>();
    case MINI_CIRCLE:
        return std::make_shared<MiniCircleVisualizer>();
    case TERRAIN_VISUALIZER_3D:
        return std::make_shared<TerrainVisualizer3D>();
    case GRID_VISUALIZER:
        return std::make_shared<GridVisualizer>();
    case SCROLLER:
        return std::make_shared<ScrollerText>();
    case CUBE:
        return std::make_shared<CubeVisualizer>();
    case MINI_CUBE:
        return std::make_shared<MiniCubeVisualizer>();
    case RACER:
        return std::make_shared<RacerVisualizer>();
    case MINI_RACER:
        return std::make_shared<MiniRacerVisualizer>();
    case MAZE:
        return std::make_shared<MazeVisualizer>();
    case HACKER:
        return std::make_shared<HackerTerminal>();
    case BALLS:
        return std::make_shared<BallsVisualizer>();
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
    else if (lowerName == "mini_bars" || lowerName == "minibars" || lowerName == "mini_bar_equalizer")
    {
        return createVisualizer(MINI_BAR_EQUALIZER);
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
    else if (lowerName == "mini_spectrogram" || lowerName == "minispectrogram" || lowerName == "mini_spectrum")
    {
        return createVisualizer(MINI_SPECTROGRAM);
    }
    else if (lowerName == "circle" || lowerName == "circles" || lowerName == "multi_band_circle")
    {
        return createVisualizer(MULTI_BAND_CIRCLE_WAVEFORM);
    }
    else if (lowerName == "mini_circle" || lowerName == "minicircle" || lowerName == "mini_circles")
    {
        return createVisualizer(MINI_CIRCLE);
    }
    else if (lowerName == "terrain" || lowerName == "3d" || lowerName == "terrain3d" || lowerName == "3d_terrain")
    {
        return createVisualizer(TERRAIN_VISUALIZER_3D);
    }
    else if (lowerName == "grid")
    {
        return createVisualizer(GRID_VISUALIZER);
    }
    else if (lowerName == "scroller" || lowerName == "text" || lowerName == "scroll")
    {
        return createVisualizer(SCROLLER);
    }
    else if (lowerName == "cube" || lowerName == "3d_cube")
    {
        return createVisualizer(CUBE);
    }
    else if (lowerName == "mini_cube" || lowerName == "minicube" || lowerName == "mini_3d_cube")
    {
        return createVisualizer(MINI_CUBE);
    }
    else if (lowerName == "racer" || lowerName == "synthwave" || lowerName == "race")
    {
        return createVisualizer(RACER);
    }
    else if (lowerName == "mini_racer" || lowerName == "miniracer")
    {
        return createVisualizer(MINI_RACER);
    }
    else if (lowerName == "maze" || lowerName == "3d_maze" || lowerName == "vector_maze")
    {
        return createVisualizer(MAZE);
    }
    else if (lowerName == "hacker" || lowerName == "terminal" || lowerName == "cyber" || lowerName == "hack")
    {
        return createVisualizer(HACKER);
    }
    else if (lowerName == "balls" || lowerName == "bouncing_balls" || lowerName == "bounce")
    {
        return createVisualizer(BALLS);
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
    case MINI_BAR_EQUALIZER:
        return "Mini Bar Equalizer";
    case WAVEFORM:
        return "Waveform";
    case MULTI_BAND_WAVEFORM:
        return "Multi-Band Waveform";
    case ASCII_BAR_EQUALIZER:
        return "ASCII Bar Equalizer";
    case SPECTROGRAM:
        return "Spectrogram";
    case MINI_SPECTROGRAM:
        return "Mini Spectrogram";
    case MULTI_BAND_CIRCLE_WAVEFORM:
        return "Multi-Band Circle Waveform";
    case MINI_CIRCLE:
        return "Mini Circle Visualizer";
    case TERRAIN_VISUALIZER_3D:
        return "3D Terrain Visualizer";
    case GRID_VISUALIZER:
        return "Grid Visualizer";
    case SCROLLER:
        return "Scroller Text";
    case CUBE:
        return "3D Cube Visualizer";
    case MINI_CUBE:
        return "Mini Cube Visualizer";
    case RACER:
        return "Synthwave Racer";
    case MINI_RACER:
        return "Mini Racer";
    case MAZE:
        return "Maze Visualizer";
    case HACKER:
        return "Hacker Terminal";
    case BALLS:
        return "Bouncing Balls";
    default:
        return "Unknown";
    }
}