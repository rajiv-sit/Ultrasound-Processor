#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <vector>

#include "ultrasound/config.hpp"
#include "ultrasound/config_io.hpp"
#include "ultrasound/processor.hpp"
#include "ultrasound/replay.hpp"
#include "ultrasound/visualizer.hpp"

int main(int argc, char** argv) {
    if (argc < 2 || argc > 4) {
        std::cerr << "Usage: uss_imgui_visualizer <input.csv> [processor_config.ini] [vehicle_config.ini]\n";
        return EXIT_FAILURE;
    }

    ultrasound::ProcessorConfig config;
    if (argc >= 3) {
        const auto status = ultrasound::load_processor_config_from_ini(argv[2], config);
        if (!status.is_ok()) {
            std::cerr << "Config load error: " << status.message << "\n";
            return EXIT_FAILURE;
        }
    } else {
        config.processing_method = ultrasound::ProcessingMethod::All;
    }

    ultrasound::UltrasoundProcessor processor(config);

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
        }
    }

    if (outputs.empty()) {
        std::cerr << "No valid frames available for visualization.\n";
        return EXIT_FAILURE;
    }

    ultrasound::VisualizerSettings settings;
    namespace fs = std::filesystem;
    fs::path vehicle_cfg_path;
    if (argc >= 4) {
        vehicle_cfg_path = fs::path(argv[3]);
    } else {
        vehicle_cfg_path = fs::path("configs") / "vehicle_profile_reference.ini";
    }
    if (fs::exists(vehicle_cfg_path)) {
        const auto geometry_status = ultrasound::load_vehicle_geometry_from_ini(vehicle_cfg_path.string(), settings.vehicle_geometry);
        if (!geometry_status.is_ok()) {
            std::cerr << "Vehicle geometry load warning: " << geometry_status.message << "\n";
        }
    } else if (argc >= 4) {
        std::cerr << "Vehicle geometry load warning: file not found: " << vehicle_cfg_path.string() << "\n";
    }

    return ultrasound::run_imgui_visualizer(outputs, settings);
}
