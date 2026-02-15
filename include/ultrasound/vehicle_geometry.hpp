#pragma once

#include <string>
#include <vector>

#include "ultrasound/error.hpp"

namespace ultrasound {

struct ContourPoint {
    float x_m{0.0F};
    float y_m{0.0F};
};

struct SensorCalibration {
    float x_m{0.0F};
    float y_m{0.0F};
    float mounting_deg{0.0F};
    float fov_deg{100.0F};
};

struct VehicleGeometry {
    std::vector<ContourPoint> contour{};
    std::vector<SensorCalibration> sensors{};
};

Status load_vehicle_geometry_from_ini(const std::string& ini_path, VehicleGeometry& geometry);

}  // namespace ultrasound
