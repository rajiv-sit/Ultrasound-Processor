#include "ultrasound/runtime.hpp"

namespace ultrasound {
namespace {

SignalWaysCallback g_signal_ways_cb{};
StaticFeaturesCallback g_static_features_cb{};
DynamicFeaturesCallback g_dynamic_features_cb{};
LineMarksCallback g_line_marks_cb{};
GridMapCallback g_grid_map_cb{};
ProcessedDetectionsCallback g_processed_cb{};

}  // namespace

RuntimeAdapterStatus query_runtime_adapter() {
    RuntimeAdapterStatus status;
    status.available = true;
    status.description = "runtime callback flow available (adapter transport remains stubbed)";
    return status;
}

void register_signal_ways_callback(SignalWaysCallback cb) {
    g_signal_ways_cb = std::move(cb);
}

void register_static_features_callback(StaticFeaturesCallback cb) {
    g_static_features_cb = std::move(cb);
}

void register_dynamic_features_callback(DynamicFeaturesCallback cb) {
    g_dynamic_features_cb = std::move(cb);
}

void register_line_marks_callback(LineMarksCallback cb) {
    g_line_marks_cb = std::move(cb);
}

void register_grid_map_callback(GridMapCallback cb) {
    g_grid_map_cb = std::move(cb);
}

void register_processed_detections_callback(ProcessedDetectionsCallback cb) {
    g_processed_cb = std::move(cb);
}

void dispatch_runtime_frame(const FrameOutput& frame) {
    if (g_signal_ways_cb) {
        g_signal_ways_cb(frame.signal_ways, frame.timestamp_us);
    }
    if (g_static_features_cb) {
        g_static_features_cb(frame.static_features, frame.timestamp_us);
    }
    if (g_dynamic_features_cb) {
        g_dynamic_features_cb(frame.dynamic_features, frame.timestamp_us);
    }
    if (g_line_marks_cb) {
        g_line_marks_cb(frame.line_marks, frame.timestamp_us);
    }
    if (g_grid_map_cb) {
        g_grid_map_cb(frame.grid_map, frame.timestamp_us);
    }
    if (g_processed_cb) {
        g_processed_cb(frame.processed, frame.timestamp_us);
    }
}

void clear_runtime_callbacks() {
    g_signal_ways_cb = {};
    g_static_features_cb = {};
    g_dynamic_features_cb = {};
    g_line_marks_cb = {};
    g_grid_map_cb = {};
    g_processed_cb = {};
}

}  // namespace ultrasound
