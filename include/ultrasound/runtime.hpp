#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "ultrasound/types.hpp"

namespace ultrasound {

struct RuntimeAdapterStatus {
    bool available{false};
    std::string description{};
};

RuntimeAdapterStatus query_runtime_adapter();

using SignalWaysCallback = std::function<void(const std::vector<SignalWay>&, std::uint64_t)>;
using StaticFeaturesCallback = std::function<void(const std::vector<StaticFeature>&, std::uint64_t)>;
using DynamicFeaturesCallback = std::function<void(const std::vector<DynamicFeature>&, std::uint64_t)>;
using LineMarksCallback = std::function<void(const std::vector<LineMark>&, std::uint64_t)>;
using GridMapCallback = std::function<void(const GridMap&, std::uint64_t)>;
using ProcessedDetectionsCallback = std::function<void(const ProcessedDetections&, std::uint64_t)>;

void register_signal_ways_callback(SignalWaysCallback cb);
void register_static_features_callback(StaticFeaturesCallback cb);
void register_dynamic_features_callback(DynamicFeaturesCallback cb);
void register_line_marks_callback(LineMarksCallback cb);
void register_grid_map_callback(GridMapCallback cb);
void register_processed_detections_callback(ProcessedDetectionsCallback cb);

void dispatch_runtime_frame(const FrameOutput& frame);
void clear_runtime_callbacks();

}  // namespace ultrasound
