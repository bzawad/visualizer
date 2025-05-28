#pragma once
#ifndef VISUALIZER_FACTORY_H
#define VISUALIZER_FACTORY_H

#include "visualizer_base.h"
#include <memory>
#include <string>
#include "racer_visualizer.h"
#include "maze_visualizer.h"

enum VisualizerType
{
    BAR_EQUALIZER,
    WAVEFORM,
    MULTI_BAND_WAVEFORM,
    ASCII_BAR_EQUALIZER,
    SPECTROGRAM,
    MULTI_BAND_CIRCLE_WAVEFORM,
    TERRAIN_VISUALIZER_3D,
    GRID_VISUALIZER,
    SCROLLER,
    CUBE,
    RACER,
    MAZE
};

class VisualizerFactory
{
public:
    // Create a visualizer of the specified type
    static std::shared_ptr<Visualizer> createVisualizer(VisualizerType type);

    // Create a visualizer from a string name
    static std::shared_ptr<Visualizer> createVisualizer(const std::string &name);

    // Get the current visualizer type as a string
    static std::string getVisualizerName(VisualizerType type);
};

#endif // VISUALIZER_FACTORY_H