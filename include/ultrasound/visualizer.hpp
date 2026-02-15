#pragma once

#include <vector>

#include "ultrasound/types.hpp"
#include "ultrasound/vehicle_geometry.hpp"

namespace ultrasound {

struct VisualizerSettings {
    bool start_paused{false};
    bool loop_playback{true};
    float playback_fps{15.0F};
    float meters_to_pixels{40.0F};
    bool show_vehicle_contour{true};
    bool show_sensors{true};
    VehicleGeometry vehicle_geometry{};
};

int run_imgui_visualizer(const std::vector<FrameOutput>& frames, const VisualizerSettings& settings = {});

}  // namespace ultrasound
