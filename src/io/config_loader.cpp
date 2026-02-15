#include "ultrasound/config_io.hpp"

#include <algorithm>
#include <cctype>
#include <exception>
#include <fstream>
#include <map>
#include <sstream>

namespace ultrasound {
namespace {

std::string trim(const std::string& value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c) != 0; });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c) != 0; }).base();
    if (first >= last) {
        return {};
    }
    return std::string(first, last);
}

bool parse_bool(const std::string& value, bool& out) {
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lower == "1" || lower == "true" || lower == "yes" || lower == "on") {
        out = true;
        return true;
    }
    if (lower == "0" || lower == "false" || lower == "no" || lower == "off") {
        out = false;
        return true;
    }
    return false;
}

bool parse_float_pair(const std::string& value, float& first, float& second) {
    std::stringstream ss(value);
    std::string left;
    std::string right;
    if (!std::getline(ss, left, ',')) {
        return false;
    }
    if (!std::getline(ss, right, ',')) {
        return false;
    }

    first = std::stof(trim(left));
    second = std::stof(trim(right));
    return true;
}

}  // namespace

Status load_processor_config_from_ini(const std::string& ini_path, ProcessorConfig& config) {
    std::ifstream in(ini_path);
    if (!in.is_open()) {
        return Status::fail(ErrorCode::InvalidInput, "unable to open config file: " + ini_path);
    }

    std::string section;
    std::string line;
    std::size_t line_number = 0U;

    while (std::getline(in, line)) {
        ++line_number;
        const std::string s = trim(line);
        if (s.empty() || s[0] == ';' || s[0] == '#') {
            continue;
        }

        if (s.front() == '[' && s.back() == ']') {
            section = trim(s.substr(1, s.size() - 2));
            continue;
        }

        const auto equal_pos = s.find('=');
        if (equal_pos == std::string::npos) {
            continue;
        }

        const std::string key = trim(s.substr(0, equal_pos));
        const std::string value = trim(s.substr(equal_pos + 1));

        try {
            if (section == "Conversion" && key == "nSigmaValeo") {
                config.n_sigma_valeo = std::stof(value);
            } else if (section == "Conversion" && key == "legacyValeoBugfix") {
                bool parsed = false;
                if (!parse_bool(value, parsed)) {
                    return Status::fail(ErrorCode::InvalidInput, "invalid bool for Conversion.legacyValeoBugfix");
                }
                config.use_legacy_valeo_bugfix = parsed;
            } else if (section == "SignalWays" && key == "groupID") {
                if (value == "FRONT" || value == "0") {
                    config.group_filter = GroupFilter::Front;
                } else if (value == "REAR" || value == "1") {
                    config.group_filter = GroupFilter::Rear;
                } else if (value == "SURROUND" || value == "2") {
                    config.group_filter = GroupFilter::Surround;
                } else {
                    return Status::fail(ErrorCode::InvalidInput, "invalid SignalWays.groupID");
                }
            } else if (section == "SignalWays" && key == "method") {
                if (value == "SIGNAL_TRACING" || value == "0") {
                    config.processing_method = ProcessingMethod::SignalTracing;
                } else if (value == "FOV_INTERSECTION" || value == "1") {
                    config.processing_method = ProcessingMethod::FovIntersection;
                } else if (value == "ELLIPSE_INTERSECTION" || value == "2") {
                    config.processing_method = ProcessingMethod::EllipseIntersection;
                } else if (value == "ALL" || value == "3") {
                    config.processing_method = ProcessingMethod::All;
                } else {
                    return Status::fail(ErrorCode::InvalidInput, "invalid SignalWays.method");
                }
            } else if (section == "SignalWays" && key == "clusterRadiusM") {
                config.cluster_radius_m = std::stof(value);
            } else if (section == "General" && key == "minRangeM") {
                config.min_range_m = std::stof(value);
            } else if (section == "General" && key == "maxRangeM") {
                config.max_range_m = std::stof(value);
            } else if (section == "General" && key == "strictMonotonicTimestamps") {
                bool parsed = false;
                if (!parse_bool(value, parsed)) {
                    return Status::fail(ErrorCode::InvalidInput, "invalid bool for General.strictMonotonicTimestamps");
                }
                config.strict_monotonic_timestamps = parsed;
            }
        } catch (const std::exception&) {
            std::ostringstream oss;
            oss << "failed parsing config at line " << line_number;
            return Status::fail(ErrorCode::InvalidInput, oss.str());
        }
    }

    if (config.min_range_m < 0.0F || config.max_range_m <= config.min_range_m || config.cluster_radius_m <= 0.0F) {
        return Status::fail(ErrorCode::InvalidInput, "invalid numeric constraints in config");
    }

    return Status::ok();
}

Status load_vehicle_geometry_from_ini(const std::string& ini_path, VehicleGeometry& geometry) {
    std::ifstream in(ini_path);
    if (!in.is_open()) {
        return Status::fail(ErrorCode::InvalidInput, "unable to open vehicle geometry file: " + ini_path);
    }

    std::string section;
    std::string line;
    std::size_t line_number = 0U;

    std::map<int, ContourPoint> contour_points;
    std::map<int, std::pair<float, float>> sensor_positions;
    std::map<int, std::pair<float, float>> sensor_mountings;

    while (std::getline(in, line)) {
        ++line_number;
        std::string s = trim(line);
        if (s.empty() || s[0] == ';' || s[0] == '#') {
            continue;
        }

        const auto semicolon = s.find(';');
        if (semicolon != std::string::npos) {
            s = trim(s.substr(0, semicolon));
            if (s.empty()) {
                continue;
            }
        }

        if (s.front() == '[' && s.back() == ']') {
            section = trim(s.substr(1, s.size() - 2));
            continue;
        }

        const auto equal_pos = s.find('=');
        if (equal_pos == std::string::npos) {
            continue;
        }

        const std::string key = trim(s.substr(0, equal_pos));
        const std::string value = trim(s.substr(equal_pos + 1));

        try {
            if (section == "Contour" && key.rfind("contourPt", 0U) == 0U) {
                const int index = std::stoi(key.substr(9));
                float x = 0.0F;
                float y = 0.0F;
                if (!parse_float_pair(value, x, y)) {
                    return Status::fail(ErrorCode::InvalidInput, "invalid contour point format at line " +
                                                                     std::to_string(line_number));
                }
                contour_points[index] = ContourPoint{x, y};
            } else if (section == "USS SENSORS" && key.rfind("uss_position_", 0U) == 0U) {
                const int index = std::stoi(key.substr(13));
                float x = 0.0F;
                float y = 0.0F;
                if (!parse_float_pair(value, x, y)) {
                    return Status::fail(ErrorCode::InvalidInput, "invalid uss_position format at line " +
                                                                     std::to_string(line_number));
                }
                sensor_positions[index] = {x, y};
            } else if (section == "USS SENSORS" && key.rfind("uss_mounting_", 0U) == 0U) {
                const int index = std::stoi(key.substr(13));
                float angle = 0.0F;
                float fov = 0.0F;
                if (!parse_float_pair(value, angle, fov)) {
                    return Status::fail(ErrorCode::InvalidInput, "invalid uss_mounting format at line " +
                                                                     std::to_string(line_number));
                }
                sensor_mountings[index] = {angle, fov};
            }
        } catch (const std::exception&) {
            return Status::fail(
                ErrorCode::InvalidInput, "failed parsing vehicle geometry at line " + std::to_string(line_number));
        }
    }

    geometry = VehicleGeometry{};
    geometry.contour.reserve(contour_points.size());
    for (const auto& [_, point] : contour_points) {
        if (point.x_m == 0.0F && point.y_m == 0.0F) {
            continue;
        }
        geometry.contour.push_back(point);
    }

    const std::size_t sensor_count = std::max(sensor_positions.size(), sensor_mountings.size());
    geometry.sensors.resize(sensor_count);
    for (std::size_t i = 0; i < sensor_count; ++i) {
        auto& s = geometry.sensors[i];
        if (sensor_positions.count(static_cast<int>(i)) > 0U) {
            const auto pos = sensor_positions[static_cast<int>(i)];
            s.x_m = pos.first;
            s.y_m = pos.second;
        }
        if (sensor_mountings.count(static_cast<int>(i)) > 0U) {
            const auto mounting = sensor_mountings[static_cast<int>(i)];
            s.mounting_deg = mounting.first;
            s.fov_deg = mounting.second;
        }
    }

    if (geometry.contour.empty() || geometry.sensors.empty()) {
        return Status::fail(ErrorCode::InvalidInput, "vehicle geometry missing contour and/or uss sensors");
    }

    return Status::ok();
}

}  // namespace ultrasound
