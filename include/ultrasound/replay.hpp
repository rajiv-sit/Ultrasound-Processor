#pragma once

#include <string>
#include <vector>

#include "ultrasound/error.hpp"
#include "ultrasound/types.hpp"

namespace ultrasound {

std::vector<FrameInput> load_replay_csv(const std::string& path);
void write_output_csv(const std::string& path, const std::vector<FrameOutput>& frames);
Status convert_legacy_capture_to_replay_csv(const std::string& input_path, const std::string& output_csv);

}  // namespace ultrasound
