#include "ultrasound/processor.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>
#include <optional>
#include <unordered_map>
#include <vector>

namespace ultrasound {
namespace {

struct SensorPose {
    double x_m{0.0};
    double y_m{0.0};
    double mounting_rad{0.0};
    double fov_rad{100.0 * (std::numbers::pi_v<double> / 180.0)};
};

struct EllipseModel {
    double cx{0.0};
    double cy{0.0};
    double axis_a{1.0};
    double axis_b{1.0};
    double theta{0.0};
};

constexpr std::array<SensorPose, 12U> kDefaultSensors{
    SensorPose{3.238, 0.913, 87.0 * (std::numbers::pi_v<double> / 180.0), 60.0 * (std::numbers::pi_v<double> / 180.0)},
    SensorPose{3.6, 0.715, 38.0 * (std::numbers::pi_v<double> / 180.0), 100.0 * (std::numbers::pi_v<double> / 180.0)},
    SensorPose{3.804, 0.276, 7.0 * (std::numbers::pi_v<double> / 180.0), 100.0 * (std::numbers::pi_v<double> / 180.0)},
    SensorPose{3.804, -0.276, -4.0 * (std::numbers::pi_v<double> / 180.0), 75.0 * (std::numbers::pi_v<double> / 180.0)},
    SensorPose{3.6, -0.715, -28.0 * (std::numbers::pi_v<double> / 180.0), 75.0 * (std::numbers::pi_v<double> / 180.0)},
    SensorPose{3.238, -0.913, -87.0 * (std::numbers::pi_v<double> / 180.0), 45.0 * (std::numbers::pi_v<double> / 180.0)},
    SensorPose{-0.775, -0.822, -100.0 * (std::numbers::pi_v<double> / 180.0), 75.0 * (std::numbers::pi_v<double> / 180.0)},
    SensorPose{-0.956, -0.71, -165.0 * (std::numbers::pi_v<double> / 180.0), 75.0 * (std::numbers::pi_v<double> / 180.0)},
    SensorPose{-1.09, -0.25, -175.0 * (std::numbers::pi_v<double> / 180.0), 75.0 * (std::numbers::pi_v<double> / 180.0)},
    SensorPose{-1.09, 0.25, 173.0 * (std::numbers::pi_v<double> / 180.0), 100.0 * (std::numbers::pi_v<double> / 180.0)},
    SensorPose{-0.956, 0.71, 151.0 * (std::numbers::pi_v<double> / 180.0), 100.0 * (std::numbers::pi_v<double> / 180.0)},
    SensorPose{-0.775, 0.822, 99.0 * (std::numbers::pi_v<double> / 180.0), 100.0 * (std::numbers::pi_v<double> / 180.0)},
};

constexpr std::array<std::array<double, 2U>, 12U> kDefaultContour{
    std::array<double, 2U>{-0.775, 0.822}, std::array<double, 2U>{-0.956, 0.71}, std::array<double, 2U>{-1.09, 0.25},
    std::array<double, 2U>{-1.09, -0.25}, std::array<double, 2U>{-0.956, -0.71}, std::array<double, 2U>{-0.775, -0.822},
    std::array<double, 2U>{3.238, -0.913}, std::array<double, 2U>{3.6, -0.715}, std::array<double, 2U>{3.804, -0.276},
    std::array<double, 2U>{3.804, 0.276}, std::array<double, 2U>{3.6, 0.715}, std::array<double, 2U>{3.238, 0.913},
};

bool group_matches(GroupFilter filter, std::uint8_t group_id) {
    if (group_id > 1U) {
        return false;
    }
    switch (filter) {
        case GroupFilter::Front:
            return group_id == 0U;
        case GroupFilter::Rear:
            return group_id == 1U;
        case GroupFilter::Surround:
            return true;
    }
    return true;
}

double sqr(double value) {
    return value * value;
}

bool map_signal_way_to_sensor_pair(std::uint8_t group_id, std::uint8_t signal_way_id, int& tx, int& rx) {
    const int base = (group_id == 1U) ? 6 : 0;
    if (group_id > 1U || signal_way_id > 15U) {
        return false;
    }

    switch (signal_way_id) {
        case 0:
            tx = base + 0;
            rx = base + 0;
            return true;
        case 1:
            tx = base + 0;
            rx = base + 1;
            return true;
        case 2:
            tx = base + 1;
            rx = base + 0;
            return true;
        case 3:
            tx = base + 1;
            rx = base + 1;
            return true;
        case 4:
            tx = base + 1;
            rx = base + 2;
            return true;
        case 5:
            tx = base + 2;
            rx = base + 1;
            return true;
        case 6:
            tx = base + 2;
            rx = base + 2;
            return true;
        case 7:
            tx = base + 2;
            rx = base + 3;
            return true;
        case 8:
            tx = base + 3;
            rx = base + 2;
            return true;
        case 9:
            tx = base + 3;
            rx = base + 3;
            return true;
        case 10:
            tx = base + 3;
            rx = base + 4;
            return true;
        case 11:
            tx = base + 4;
            rx = base + 3;
            return true;
        case 12:
            tx = base + 4;
            rx = base + 4;
            return true;
        case 13:
            tx = base + 4;
            rx = base + 5;
            return true;
        case 14:
            tx = base + 5;
            rx = base + 4;
            return true;
        case 15:
            tx = base + 5;
            rx = base + 5;
            return true;
        default:
            return false;
    }
}

bool is_inside_vehicle_contour(double x_m, double y_m) {
    bool inside = false;
    for (std::size_t i = 0, j = kDefaultContour.size() - 1U; i < kDefaultContour.size(); j = i++) {
        const double xi = kDefaultContour[i][0];
        const double yi = kDefaultContour[i][1];
        const double xj = kDefaultContour[j][0];
        const double yj = kDefaultContour[j][1];

        const bool intersect = ((yi > y_m) != (yj > y_m)) &&
                               (x_m < (xj - xi) * (y_m - yi) / ((yj - yi) + std::numeric_limits<double>::epsilon()) + xi);
        if (intersect) {
            inside = !inside;
        }
    }
    return inside;
}

std::array<double, 2U> ellipse_point(const EllipseModel& e, double param_t) {
    const double ct = std::cos(param_t);
    const double st = std::sin(param_t);
    const double cp = std::cos(e.theta);
    const double sp = std::sin(e.theta);

    const double x_local = e.axis_a * ct;
    const double y_local = e.axis_b * st;
    return {e.cx + x_local * cp - y_local * sp, e.cy + x_local * sp + y_local * cp};
}

double ellipse_implicit_error(const EllipseModel& e, double x_m, double y_m) {
    const double dx = x_m - e.cx;
    const double dy = y_m - e.cy;
    const double cp = std::cos(e.theta);
    const double sp = std::sin(e.theta);
    const double xr = dx * cp + dy * sp;
    const double yr = -dx * sp + dy * cp;
    const double v = sqr(xr) / std::max(sqr(e.axis_a), 1.0e-9) + sqr(yr) / std::max(sqr(e.axis_b), 1.0e-9);
    return std::fabs(v - 1.0);
}

void push_unique_detection(std::vector<std::array<double, 2U>>& detections, const std::array<double, 2U>& candidate) {
    constexpr double kMinSepSq = 0.08 * 0.08;
    for (const auto& p : detections) {
        const double dx = p[0] - candidate[0];
        const double dy = p[1] - candidate[1];
        if ((dx * dx + dy * dy) <= kMinSepSq) {
            return;
        }
    }
    detections.push_back(candidate);
}

std::optional<EllipseModel> build_ellipse_from_signal_way(const SignalWay& sw) {
    int tx = 0;
    int rx = 0;
    if (!map_signal_way_to_sensor_pair(sw.group_id, sw.signal_way_id, tx, rx)) {
        return std::nullopt;
    }

    if (tx < 0 || rx < 0 || tx >= static_cast<int>(kDefaultSensors.size()) || rx >= static_cast<int>(kDefaultSensors.size())) {
        return std::nullopt;
    }

    const auto& s0 = kDefaultSensors[tx];
    const auto& s1 = kDefaultSensors[rx];
    const double distance = static_cast<double>(sw.distance_m);
    if (distance <= 0.0) {
        return std::nullopt;
    }

    const double dx = s1.x_m - s0.x_m;
    const double dy = s1.y_m - s0.y_m;
    const double sensor_distance = std::sqrt(dx * dx + dy * dy);
    const double half_sensor_distance = 0.5 * sensor_distance;

    if (distance <= half_sensor_distance) {
        return std::nullopt;
    }

    EllipseModel model;
    model.cx = 0.5 * (s0.x_m + s1.x_m);
    model.cy = 0.5 * (s0.y_m + s1.y_m);
    model.axis_a = distance;
    model.axis_b = std::sqrt(std::max(0.0, distance * distance - half_sensor_distance * half_sensor_distance));
    model.theta = std::atan2(dy, dx);
    return model;
}

std::optional<EllipseModel> build_fov_model_from_signal_way(const SignalWay& sw) {
    int tx = 0;
    int rx = 0;
    if (!map_signal_way_to_sensor_pair(sw.group_id, sw.signal_way_id, tx, rx)) {
        return std::nullopt;
    }
    if (tx < 0 || rx < 0 || tx >= static_cast<int>(kDefaultSensors.size()) || rx >= static_cast<int>(kDefaultSensors.size())) {
        return std::nullopt;
    }

    const auto& s0 = kDefaultSensors[tx];
    const auto& s1 = kDefaultSensors[rx];
    const double distance = static_cast<double>(sw.distance_m);
    if (distance <= 0.0) {
        return std::nullopt;
    }

    EllipseModel model;
    model.cx = 0.5 * (s0.x_m + s1.x_m);
    model.cy = 0.5 * (s0.y_m + s1.y_m);

    if (tx == rx) {
        model.axis_a = distance;
        model.axis_b = distance;
        model.theta = s0.mounting_rad;
    } else {
        const double baseline = std::sqrt(sqr(s1.x_m - s0.x_m) + sqr(s1.y_m - s0.y_m));
        model.axis_a = distance;
        model.axis_b = std::max(0.25 * distance, 0.5 * baseline);
        model.theta = 0.5 * (s0.mounting_rad + s1.mounting_rad);
    }
    return model;
}

std::array<double, 2U> tracing_detection_from_signal_way(const SignalWay& sw) {
    int tx = 0;
    int rx = 0;
    if (!map_signal_way_to_sensor_pair(sw.group_id, sw.signal_way_id, tx, rx)) {
        return {static_cast<double>(sw.distance_m), sw.group_id == 0U ? 1.0 : -1.0};
    }
    if (tx < 0 || rx < 0 || tx >= static_cast<int>(kDefaultSensors.size()) || rx >= static_cast<int>(kDefaultSensors.size())) {
        return {static_cast<double>(sw.distance_m), sw.group_id == 0U ? 1.0 : -1.0};
    }

    const auto& s0 = kDefaultSensors[tx];
    const auto& s1 = kDefaultSensors[rx];
    const double distance = static_cast<double>(sw.distance_m);
    const double vx0 = std::cos(s0.mounting_rad) * distance;
    const double vy0 = std::sin(s0.mounting_rad) * distance;
    const double vx1 = std::cos(s1.mounting_rad) * distance;
    const double vy1 = std::sin(s1.mounting_rad) * distance;

    double vx = vx0 + vx1;
    double vy = vy0 + vy1;
    const double norm = std::sqrt(vx * vx + vy * vy);
    if (norm > 1.0e-9) {
        vx = distance * (vx / norm);
        vy = distance * (vy / norm);
    }

    const double cx = 0.5 * (s0.x_m + s1.x_m);
    const double cy = 0.5 * (s0.y_m + s1.y_m);
    return {cx + vx, cy + vy};
}

std::array<double, 2U> fov_detection_from_signal_way(const SignalWay& sw) {
    const auto tracing = tracing_detection_from_signal_way(sw);
    return {tracing[0] * 0.98, tracing[1] * 0.98};
}

double wrap_to_pi(double angle) {
    while (angle > std::numbers::pi_v<double>) {
        angle -= 2.0 * std::numbers::pi_v<double>;
    }
    while (angle < -std::numbers::pi_v<double>) {
        angle += 2.0 * std::numbers::pi_v<double>;
    }
    return angle;
}

bool point_in_sensor_sector(const SensorPose& s, const std::array<double, 2U>& p, double range_m) {
    const double dx = p[0] - s.x_m;
    const double dy = p[1] - s.y_m;
    const double r = std::sqrt(dx * dx + dy * dy);
    if (r > range_m + 1.0e-6) {
        return false;
    }
    const double bearing = std::atan2(dy, dx);
    const double delta = std::fabs(wrap_to_pi(bearing - s.mounting_rad));
    return delta <= (0.5 * s.fov_rad + 1.0e-6);
}

bool ray_intersection(const std::array<double, 2U>& p0,
                      const std::array<double, 2U>& d0,
                      const std::array<double, 2U>& p1,
                      const std::array<double, 2U>& d1,
                      std::array<double, 2U>& out) {
    const double det = d0[0] * d1[1] - d0[1] * d1[0];
    if (std::fabs(det) < 1.0e-6) {
        return false;
    }

    const double px = p1[0] - p0[0];
    const double py = p1[1] - p0[1];
    const double t = (px * d1[1] - py * d1[0]) / det;
    const double u = (px * d0[1] - py * d0[0]) / det;
    if (t < 0.0 || u < 0.0) {
        return false;
    }

    out = {p0[0] + t * d0[0], p0[1] + t * d0[1]};
    return true;
}

std::optional<std::array<double, 2U>> fov_pie_detection(const SignalWay& sw) {
    int tx = 0;
    int rx = 0;
    if (!map_signal_way_to_sensor_pair(sw.group_id, sw.signal_way_id, tx, rx)) {
        return std::nullopt;
    }
    if (tx < 0 || rx < 0 || tx >= static_cast<int>(kDefaultSensors.size()) || rx >= static_cast<int>(kDefaultSensors.size())) {
        return std::nullopt;
    }

    const auto& s0 = kDefaultSensors[tx];
    const auto& s1 = kDefaultSensors[rx];
    const double range_m = static_cast<double>(sw.distance_m);
    if (range_m <= 0.0) {
        return std::nullopt;
    }

    // Monostatic: detection at the middle of the sensor's FOV arc.
    if (tx == rx) {
        return std::array<double, 2U>{
            s0.x_m + range_m * std::cos(s0.mounting_rad),
            s0.y_m + range_m * std::sin(s0.mounting_rad)};
    }

    // Bistatic: source location approximated by intersection of both sensors center rays.
    const std::array<double, 2U> p0{s0.x_m, s0.y_m};
    const std::array<double, 2U> d0{std::cos(s0.mounting_rad), std::sin(s0.mounting_rad)};
    const std::array<double, 2U> p1{s1.x_m, s1.y_m};
    const std::array<double, 2U> d1{std::cos(s1.mounting_rad), std::sin(s1.mounting_rad)};
    std::array<double, 2U> candidate{};
    if (ray_intersection(p0, d0, p1, d1, candidate) &&
        point_in_sensor_sector(s0, candidate, range_m) &&
        point_in_sensor_sector(s1, candidate, range_m)) {
        return candidate;
    }

    // Fallback when center rays don't intersect in valid sectors.
    return fov_detection_from_signal_way(sw);
}

void collect_ellipse_intersections(const std::vector<EllipseModel>& models,
                                   std::vector<std::array<double, 2U>>& out,
                                   double tolerance,
                                   double best_limit) {
    if (models.size() < 2U) {
        return;
    }
    constexpr int kSamples = 360;
    for (std::size_t i = 0; i + 1U < models.size(); ++i) {
        for (std::size_t j = i + 1U; j < models.size(); ++j) {
            double best_err = std::numeric_limits<double>::max();
            std::array<double, 2U> best_pt{};
            for (int s = 0; s < kSamples; ++s) {
                const double t = (static_cast<double>(s) / static_cast<double>(kSamples)) *
                                 (2.0 * std::numbers::pi_v<double>);
                const auto p = ellipse_point(models[i], t);
                const double err = ellipse_implicit_error(models[j], p[0], p[1]);
                if (err < best_err) {
                    best_err = err;
                    best_pt = p;
                }
                if (err <= tolerance && !is_inside_vehicle_contour(p[0], p[1])) {
                    push_unique_detection(out, p);
                }
            }
            if (best_err <= best_limit && !is_inside_vehicle_contour(best_pt[0], best_pt[1])) {
                push_unique_detection(out, best_pt);
            }
        }
    }
}

double ellipse_implicit_value(const EllipseModel& e, double x_m, double y_m) {
    const double dx = x_m - e.cx;
    const double dy = y_m - e.cy;
    const double cp = std::cos(e.theta);
    const double sp = std::sin(e.theta);
    const double xr = dx * cp + dy * sp;
    const double yr = -dx * sp + dy * cp;
    return sqr(xr) / std::max(sqr(e.axis_a), 1.0e-9) + sqr(yr) / std::max(sqr(e.axis_b), 1.0e-9) - 1.0;
}

// Legacy-style traverse approximation: march along one ellipse and locate sign changes w.r.t. the other implicit equation.
void collect_ellipse_intersections_traverse(const std::vector<EllipseModel>& models,
                                            std::vector<std::array<double, 2U>>& out) {
    if (models.size() < 2U) {
        return;
    }

    constexpr int kSamples = 360;
    for (std::size_t i = 0; i + 1U < models.size(); ++i) {
        for (std::size_t j = i + 1U; j < models.size(); ++j) {
            double prev_t = 0.0;
            auto prev_p = ellipse_point(models[i], prev_t);
            double prev_v = ellipse_implicit_value(models[j], prev_p[0], prev_p[1]);

            for (int s = 1; s <= kSamples; ++s) {
                const double t = (static_cast<double>(s) / static_cast<double>(kSamples)) *
                                 (2.0 * std::numbers::pi_v<double>);
                const auto p = ellipse_point(models[i], t);
                const double v = ellipse_implicit_value(models[j], p[0], p[1]);

                if ((prev_v <= 0.0 && v >= 0.0) || (prev_v >= 0.0 && v <= 0.0)) {
                    double lo = prev_t;
                    double hi = t;
                    for (int it = 0; it < 20; ++it) {
                        const double mid = 0.5 * (lo + hi);
                        const auto mid_p = ellipse_point(models[i], mid);
                        const double mid_v = ellipse_implicit_value(models[j], mid_p[0], mid_p[1]);
                        if ((prev_v <= 0.0 && mid_v >= 0.0) || (prev_v >= 0.0 && mid_v <= 0.0)) {
                            hi = mid;
                        } else {
                            lo = mid;
                            prev_v = mid_v;
                        }
                    }
                    const auto root_p = ellipse_point(models[i], 0.5 * (lo + hi));
                    if (!is_inside_vehicle_contour(root_p[0], root_p[1])) {
                        push_unique_detection(out, root_p);
                    }
                }

                prev_t = t;
                prev_p = p;
                prev_v = v;
            }
        }
    }
}

double point_distance_sq(const std::array<double, 2U>& a, const std::array<double, 2U>& b) {
    return sqr(a[0] - b[0]) + sqr(a[1] - b[1]);
}

bool has_support_near(const std::vector<std::array<double, 2U>>& detections,
                      const std::array<double, 2U>& candidate,
                      double radius_m) {
    const double radius_sq = radius_m * radius_m;
    for (const auto& p : detections) {
        if (point_distance_sq(p, candidate) <= radius_sq) {
            return true;
        }
    }
    return false;
}

std::vector<std::array<double, 2U>> fuse_method_detections(const ProcessedDetections& in) {
    std::vector<std::array<double, 2U>> candidates;
    candidates.reserve(in.tracing.size() + in.fov_intersections.size() + in.ellipse_intersections.size());
    for (const auto& p : in.tracing) {
        push_unique_detection(candidates, p);
    }
    for (const auto& p : in.fov_intersections) {
        push_unique_detection(candidates, p);
    }
    for (const auto& p : in.ellipse_intersections) {
        push_unique_detection(candidates, p);
    }

    const bool has_tracing = !in.tracing.empty();
    const bool has_fov = !in.fov_intersections.empty();
    const bool has_ellipse = !in.ellipse_intersections.empty();
    const int available_methods = (has_tracing ? 1 : 0) + (has_fov ? 1 : 0) + (has_ellipse ? 1 : 0);

    std::vector<std::array<double, 2U>> fused;
    fused.reserve(candidates.size());

    // FOV acts as existence verification for other methods when available.
    constexpr double kSupportRadiusM = 0.55;
    for (const auto& c : candidates) {
        const bool support_tracing = has_support_near(in.tracing, c, kSupportRadiusM);
        const bool support_fov = has_support_near(in.fov_intersections, c, kSupportRadiusM);
        const bool support_ellipse = has_support_near(in.ellipse_intersections, c, kSupportRadiusM);
        const int support_count = (support_tracing ? 1 : 0) + (support_fov ? 1 : 0) + (support_ellipse ? 1 : 0);

        if (available_methods <= 1) {
            push_unique_detection(fused, c);
            continue;
        }

        if (support_count >= 2) {
            push_unique_detection(fused, c);
        }
    }

    // Fallback: if cross-method agreement is unavailable, keep best available method detections.
    if (fused.empty()) {
        if (has_fov) {
            for (const auto& p : in.fov_intersections) {
                push_unique_detection(fused, p);
            }
        }
        if (fused.empty() && has_ellipse) {
            for (const auto& p : in.ellipse_intersections) {
                push_unique_detection(fused, p);
            }
        }
        if (fused.empty() && has_tracing) {
            for (const auto& p : in.tracing) {
                push_unique_detection(fused, p);
            }
        }
    }

    return fused;
}

std::vector<std::array<double, 2U>> cluster_with_table_melt(const std::vector<std::array<double, 2U>>& in, double radius_m) {
    std::vector<std::array<double, 2U>> clustered;
    if (in.empty()) {
        return clustered;
    }

    const double radius_sq = radius_m * radius_m;
    const std::size_t n = in.size();
    std::vector<std::vector<std::uint8_t>> adjacency(n, std::vector<std::uint8_t>(n, 0U));
    for (std::size_t i = 0; i < n; ++i) {
        adjacency[i][i] = 1U;
        for (std::size_t j = i + 1U; j < n; ++j) {
            const double dx = in[j][0] - in[i][0];
            const double dy = in[j][1] - in[i][1];
            if ((dx * dx + dy * dy) <= radius_sq) {
                adjacency[i][j] = 1U;
                adjacency[j][i] = 1U;
            }
        }
    }

    std::vector<int> cluster_id(n, 0);
    int next_id = 1;
    for (std::size_t i = 0; i < n; ++i) {
        if (cluster_id[i] != 0) {
            continue;
        }
        cluster_id[i] = next_id;
        bool changed = true;
        while (changed) {
            changed = false;
            for (std::size_t a = 0; a < n; ++a) {
                if (cluster_id[a] != next_id) {
                    continue;
                }
                for (std::size_t b = 0; b < n; ++b) {
                    if (adjacency[a][b] != 0U && cluster_id[b] == 0) {
                        cluster_id[b] = next_id;
                        changed = true;
                    }
                }
            }
        }
        ++next_id;
    }

    std::unordered_map<int, std::array<double, 3U>> accum;
    for (std::size_t i = 0; i < n; ++i) {
        auto& a = accum[cluster_id[i]];
        a[0] += in[i][0];
        a[1] += in[i][1];
        a[2] += 1.0;
    }

    clustered.reserve(accum.size());
    for (int cid = 1; cid < next_id; ++cid) {
        const auto it = accum.find(cid);
        if (it == accum.end() || it->second[2] <= 0.0) {
            continue;
        }
        clustered.push_back({it->second[0] / it->second[2], it->second[1] / it->second[2]});
    }
    return clustered;
}

}  // namespace

UltrasoundProcessor::UltrasoundProcessor(ProcessorConfig config)
    : config_(config) {}

Status UltrasoundProcessor::push_vehicle_state(const VehicleState& state) {
    if (!state_queue_.empty() && state.timestamp_us <= state_queue_.back().timestamp_us) {
        return Status::fail(ErrorCode::InvalidInput, "vehicle state timestamps must be monotonic");
    }

    state_queue_.push_back(state);
    constexpr std::size_t kMaxQueue = 64U;
    while (state_queue_.size() > kMaxQueue) {
        state_queue_.pop_front();
    }
    return Status::ok();
}

Status UltrasoundProcessor::process_frame(const FrameInput& input) {
    const auto t0 = std::chrono::steady_clock::now();
    diagnostics_.last_stage_timing_us = {};

    if (config_.strict_monotonic_timestamps && input.timestamp_us <= last_timestamp_us_) {
        ++diagnostics_.dropped_frames;
        ++diagnostics_.out_of_order_frames;
        return Status::fail(ErrorCode::OutOfOrderTimestamp, "frame timestamp out of order");
    }

    if (input.signal_ways.empty() && input.static_features.empty()) {
        ++diagnostics_.dropped_frames;
        ++diagnostics_.invalid_input_frames;
        return Status::fail(ErrorCode::InvalidInput, "frame has no signal ways or static features");
    }
    const auto t_decode_end = std::chrono::steady_clock::now();

    const auto t_interpolate_start = std::chrono::steady_clock::now();
    const auto pose = interpolate_pose(input.timestamp_us);
    if (!pose.has_value()) {
        ++diagnostics_.dropped_frames;
        ++diagnostics_.missing_state_frames;
        return Status::fail(ErrorCode::MissingVehicleState, "no vehicle state available for frame");
    }
    const auto t_interpolate_end = std::chrono::steady_clock::now();

    const auto t_convert_start = std::chrono::steady_clock::now();
    FrameOutput output;
    output.timestamp_us = input.timestamp_us;
    output.observation_pose = *pose;

    for (const auto& sw : input.signal_ways) {
        const bool range_ok = sw.distance_m > config_.min_range_m && sw.distance_m <= config_.max_range_m;
        const bool group_ok = group_matches(config_.group_filter, sw.group_id);
        if (range_ok && group_ok) {
            output.signal_ways.push_back(sw);
        } else {
            ++diagnostics_.filtered_signal_ways;
        }
    }

    for (const auto& sf : input.static_features) {
        if (sf.valid) {
            output.static_features.push_back(sf);
        }
    }
    for (const auto& df : input.dynamic_features) {
        if (df.valid) {
            output.dynamic_features.push_back(df);
        }
    }
    for (const auto& lm : input.line_marks) {
        if (lm.valid) {
            output.line_marks.push_back(lm);
        }
    }
    output.grid_map = input.grid_map;
    const auto t_convert_end = std::chrono::steady_clock::now();

    const auto t_postprocess_start = std::chrono::steady_clock::now();
    output.processed = post_process(output.signal_ways);
    const auto t_postprocess_end = std::chrono::steady_clock::now();

    const auto t_publish_start = std::chrono::steady_clock::now();
    last_output_ = output;
    last_timestamp_us_ = input.timestamp_us;
    ++diagnostics_.processed_frames;
    diagnostics_.clustered_detections += output.processed.clustered.size();
    const auto t_publish_end = std::chrono::steady_clock::now();

    diagnostics_.last_stage_timing_us.decode =
        static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(t_decode_end - t0).count());
    diagnostics_.last_stage_timing_us.interpolate = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(t_interpolate_end - t_interpolate_start).count());
    diagnostics_.last_stage_timing_us.convert = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(t_convert_end - t_convert_start).count());
    diagnostics_.last_stage_timing_us.postprocess = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(t_postprocess_end - t_postprocess_start).count());
    diagnostics_.last_stage_timing_us.publish = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(t_publish_end - t_publish_start).count());

    diagnostics_.cumulative_stage_timing_us.decode += diagnostics_.last_stage_timing_us.decode;
    diagnostics_.cumulative_stage_timing_us.interpolate += diagnostics_.last_stage_timing_us.interpolate;
    diagnostics_.cumulative_stage_timing_us.convert += diagnostics_.last_stage_timing_us.convert;
    diagnostics_.cumulative_stage_timing_us.postprocess += diagnostics_.last_stage_timing_us.postprocess;
    diagnostics_.cumulative_stage_timing_us.publish += diagnostics_.last_stage_timing_us.publish;

    return Status::ok();
}

std::optional<FrameOutput> UltrasoundProcessor::last_output() const {
    return last_output_;
}

Diagnostics UltrasoundProcessor::diagnostics() const {
    return diagnostics_;
}

std::optional<Pose2d> UltrasoundProcessor::interpolate_pose(std::uint64_t timestamp_us) const {
    if (state_queue_.empty()) {
        return std::nullopt;
    }

    if (timestamp_us <= state_queue_.front().timestamp_us) {
        return state_queue_.front().pose;
    }

    if (timestamp_us >= state_queue_.back().timestamp_us) {
        return state_queue_.back().pose;
    }

    for (std::size_t i = 1; i < state_queue_.size(); ++i) {
        const auto& prev = state_queue_[i - 1U];
        const auto& next = state_queue_[i];
        if (timestamp_us <= next.timestamp_us) {
            const auto dt = static_cast<double>(next.timestamp_us - prev.timestamp_us);
            const auto alpha = static_cast<double>(timestamp_us - prev.timestamp_us) / dt;

            Pose2d p;
            p.x_m = static_cast<float>((1.0 - alpha) * prev.pose.x_m + alpha * next.pose.x_m);
            p.y_m = static_cast<float>((1.0 - alpha) * prev.pose.y_m + alpha * next.pose.y_m);
            p.yaw_rad = static_cast<float>((1.0 - alpha) * prev.pose.yaw_rad + alpha * next.pose.yaw_rad);
            return p;
        }
    }

    return std::nullopt;
}

ProcessedDetections UltrasoundProcessor::post_process(const std::vector<SignalWay>& signal_ways) const {
    ProcessedDetections out;
    std::vector<EllipseModel> ellipses;
    std::vector<EllipseModel> fov_models;
    ellipses.reserve(signal_ways.size());
    fov_models.reserve(signal_ways.size());

    for (const auto& sw : signal_ways) {
        if (config_.processing_method == ProcessingMethod::SignalTracing ||
            config_.processing_method == ProcessingMethod::All) {
            out.tracing.push_back(tracing_detection_from_signal_way(sw));
        }

        if (config_.processing_method == ProcessingMethod::FovIntersection ||
            config_.processing_method == ProcessingMethod::All) {
            if (const auto fov_pt = fov_pie_detection(sw); fov_pt.has_value()) {
                out.fov_intersections.push_back(*fov_pt);
            }
            if (const auto fov = build_fov_model_from_signal_way(sw); fov.has_value()) {
                fov_models.push_back(*fov);
            }
        }

        if (config_.processing_method == ProcessingMethod::EllipseIntersection ||
            config_.processing_method == ProcessingMethod::All) {
            if (const auto ellipse = build_ellipse_from_signal_way(sw); ellipse.has_value()) {
                ellipses.push_back(*ellipse);
                const auto seed = ellipse_point(*ellipse, 0.30 * std::numbers::pi_v<double>);
                if (!is_inside_vehicle_contour(seed[0], seed[1])) {
                    out.ellipse_intersections.push_back(seed);
                }
            }
        }
    }

    if ((config_.processing_method == ProcessingMethod::EllipseIntersection ||
         config_.processing_method == ProcessingMethod::All) &&
        ellipses.size() > 1U) {
        collect_ellipse_intersections_traverse(ellipses, out.ellipse_intersections);
        collect_ellipse_intersections(ellipses, out.ellipse_intersections, 0.08, 0.2);
    }

    if ((config_.processing_method == ProcessingMethod::FovIntersection ||
         config_.processing_method == ProcessingMethod::All) &&
        fov_models.size() > 1U) {
        collect_ellipse_intersections(fov_models, out.fov_intersections, 0.10, 0.25);
    }

    out.fused = fuse_method_detections(out);

    out.clustered = cluster_with_table_melt(out.fused, static_cast<double>(config_.cluster_radius_m));

    return out;
}

}  // namespace ultrasound
