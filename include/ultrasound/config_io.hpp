#pragma once

#include <string>

#include "ultrasound/config.hpp"
#include "ultrasound/error.hpp"
#include "ultrasound/vehicle_geometry.hpp"

namespace ultrasound {

Status load_processor_config_from_ini(const std::string& ini_path, ProcessorConfig& config);

}  // namespace ultrasound
