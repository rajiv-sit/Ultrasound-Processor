#pragma once

#include <cstdint>

namespace ultrasound {

struct StageTimingUs {
    std::uint64_t decode{0U};
    std::uint64_t interpolate{0U};
    std::uint64_t convert{0U};
    std::uint64_t postprocess{0U};
    std::uint64_t publish{0U};
};

struct Diagnostics {
    std::uint64_t processed_frames{0U};
    std::uint64_t dropped_frames{0U};
    std::uint64_t out_of_order_frames{0U};
    std::uint64_t missing_state_frames{0U};
    std::uint64_t invalid_input_frames{0U};
    std::uint64_t filtered_signal_ways{0U};
    std::uint64_t clustered_detections{0U};
    StageTimingUs last_stage_timing_us{};
    StageTimingUs cumulative_stage_timing_us{};
    bool replay_mode{true};
    bool realtime_mode{false};
};

}  // namespace ultrasound
