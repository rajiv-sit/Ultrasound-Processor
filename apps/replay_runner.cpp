#include <cstdlib>
#include <iostream>
#include <vector>

#include "ultrasound/config.hpp"
#include "ultrasound/config_io.hpp"
#include "ultrasound/processor.hpp"
#include "ultrasound/replay.hpp"
#include "ultrasound/runtime.hpp"

int main(int argc, char** argv) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: uss_replay_runner <input.csv> <output.csv> [config.ini]\n";
        return EXIT_FAILURE;
    }

    ultrasound::ProcessorConfig config;
    if (argc == 4) {
        const auto load_status = ultrasound::load_processor_config_from_ini(argv[3], config);
        if (!load_status.is_ok()) {
            std::cerr << "Config load error: " << load_status.message << "\n";
            return EXIT_FAILURE;
        }
    }
    ultrasound::UltrasoundProcessor processor(config);

    std::uint64_t callback_frames = 0U;
    ultrasound::register_processed_detections_callback(
        [&callback_frames](const ultrasound::ProcessedDetections&, std::uint64_t) { ++callback_frames; });

    // Seed deterministic vehicle states for replay demonstration.
    for (std::uint64_t t = 0; t <= 5'000'000; t += 50'000) {
        ultrasound::VehicleState state;
        state.timestamp_us = t;
        state.pose.x_m = static_cast<float>(t) * 1.0e-6F;
        state.pose.y_m = 0.0F;
        state.pose.yaw_rad = 0.0F;
        (void)processor.push_vehicle_state(state);
    }

    const auto frames = ultrasound::load_replay_csv(argv[1]);
    std::vector<ultrasound::FrameOutput> outputs;
    outputs.reserve(frames.size());

    for (const auto& frame : frames) {
        const auto status = processor.process_frame(frame);
        if (!status.is_ok()) {
            std::cerr << "Dropped frame @" << frame.timestamp_us << " reason=" << status.message << "\n";
            continue;
        }
        const auto out = processor.last_output();
        if (out.has_value()) {
            outputs.push_back(*out);
            ultrasound::dispatch_runtime_frame(*out);
        }
    }

    ultrasound::write_output_csv(argv[2], outputs);

    const auto diag = processor.diagnostics();
    std::cout << "processed=" << diag.processed_frames << " dropped=" << diag.dropped_frames << "\n";
    std::cout << "filtered_signal_ways=" << diag.filtered_signal_ways
              << " clustered_detections=" << diag.clustered_detections << "\n";
    std::cout << "last_stage_us decode=" << diag.last_stage_timing_us.decode
              << " interp=" << diag.last_stage_timing_us.interpolate
              << " convert=" << diag.last_stage_timing_us.convert
              << " post=" << diag.last_stage_timing_us.postprocess
              << " publish=" << diag.last_stage_timing_us.publish << "\n";
    const auto runtime_status = ultrasound::query_runtime_adapter();
    std::cout << "runtime_adapter_available=" << (runtime_status.available ? "true" : "false")
              << " info=\"" << runtime_status.description << "\"\n";
    std::cout << "runtime_callbacks_dispatched=" << callback_frames << "\n";
    ultrasound::clear_runtime_callbacks();
    return EXIT_SUCCESS;
}
