#pragma once

#include <deque>
#include <optional>

#include "ultrasound/config.hpp"
#include "ultrasound/diagnostics.hpp"
#include "ultrasound/error.hpp"
#include "ultrasound/types.hpp"

namespace ultrasound {

class UltrasoundProcessor {
  public:
    explicit UltrasoundProcessor(ProcessorConfig config = ProcessorConfig{});

    Status push_vehicle_state(const VehicleState& state);
    Status process_frame(const FrameInput& input);

    std::optional<FrameOutput> last_output() const;
    Diagnostics diagnostics() const;

  private:
    std::optional<Pose2d> interpolate_pose(std::uint64_t timestamp_us) const;
    ProcessedDetections post_process(const std::vector<SignalWay>& signal_ways) const;

    ProcessorConfig config_{};
    Diagnostics diagnostics_{};
    std::deque<VehicleState> state_queue_{};
    std::optional<FrameOutput> last_output_{};
    std::uint64_t last_timestamp_us_{0U};
};

}  // namespace ultrasound
