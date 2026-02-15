#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ultrasound {

struct Pose2d {
    float x_m{0.0F};
    float y_m{0.0F};
    float yaw_rad{0.0F};
};

struct VehicleState {
    std::uint64_t timestamp_us{0U};
    Pose2d pose{};
    float v_lon_mps{0.0F};
    float yaw_rate_rps{0.0F};
};

struct SignalWay {
    std::uint64_t timestamp_us{0U};
    float distance_m{0.0F};
    std::uint8_t group_id{0U};
    std::uint8_t signal_way_id{0U};
};

struct StaticFeature {
    float x_m{0.0F};
    float y_m{0.0F};
    float std_x_m{0.0F};
    float std_y_m{0.0F};
    float angle_rad{0.0F};
    float existence{0.0F};
    float free_prob{0.0F};
    std::uint8_t height{0U};
    std::uint8_t track_state{0U};
    std::uint8_t source_mask{0U};
    bool valid{false};
};

struct DynamicFeature {
    float x_m{0.0F};
    float y_m{0.0F};
    float vx_mps{0.0F};
    float vy_mps{0.0F};
    bool valid{false};
};

struct LineMark {
    float x0_m{0.0F};
    float y0_m{0.0F};
    float x1_m{0.0F};
    float y1_m{0.0F};
    bool valid{false};
};

struct GridMap {
    std::uint32_t rows{0U};
    std::uint32_t cols{0U};
    float cell_size_m{0.0F};
    float origin_x_m{0.0F};
    float origin_y_m{0.0F};
    std::vector<float> occupancy{};
    bool valid{false};
};

struct ProcessedDetections {
    std::vector<std::array<double, 2U>> tracing;
    std::vector<std::array<double, 2U>> fov_intersections;
    std::vector<std::array<double, 2U>> ellipse_intersections;
    std::vector<std::array<double, 2U>> fused;
    std::vector<std::array<double, 2U>> clustered;
};

struct FrameInput {
    std::uint64_t timestamp_us{0U};
    std::vector<SignalWay> signal_ways;
    std::vector<StaticFeature> static_features;
    std::vector<DynamicFeature> dynamic_features;
    std::vector<LineMark> line_marks;
    GridMap grid_map{};
};

struct FrameOutput {
    std::uint64_t timestamp_us{0U};
    Pose2d observation_pose{};
    std::vector<SignalWay> signal_ways;
    std::vector<StaticFeature> static_features;
    std::vector<DynamicFeature> dynamic_features;
    std::vector<LineMark> line_marks;
    GridMap grid_map{};
    ProcessedDetections processed{};
};

}  // namespace ultrasound
