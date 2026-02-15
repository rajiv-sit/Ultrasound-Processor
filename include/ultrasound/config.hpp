#pragma once

#include <cstdint>
#include <string>

namespace ultrasound {

enum class GroupFilter : std::uint8_t {
    Front = 0,
    Rear = 1,
    Surround = 2
};

enum class ProcessingMethod : std::uint8_t {
    SignalTracing = 0,
    FovIntersection = 1,
    EllipseIntersection = 2,
    All = 3
};

struct ProcessorConfig {
    float n_sigma_valeo{3.0F};
    bool use_legacy_valeo_bugfix{false};
    GroupFilter group_filter{GroupFilter::Surround};
    ProcessingMethod processing_method{ProcessingMethod::All};
    float min_range_m{0.00001F};
    float max_range_m{5.5F};
    float cluster_radius_m{0.35F};
    bool strict_monotonic_timestamps{true};
};

struct ReplayConfig {
    std::string input_csv{};
    std::string output_csv{};
};

}  // namespace ultrasound
