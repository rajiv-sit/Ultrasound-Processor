#include "ultrasound/visualizer.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.hpp"
#include "imgui_impl_opengl3.hpp"

namespace ultrasound {
namespace {

constexpr std::array<float, 4U> kColorTracing{0.96F, 0.35F, 0.26F, 1.0F};
constexpr std::array<float, 4U> kColorFov{0.26F, 0.76F, 0.96F, 1.0F};
constexpr std::array<float, 4U> kColorEllipse{0.96F, 0.72F, 0.26F, 1.0F};
constexpr std::array<float, 4U> kColorFused{0.33F, 0.86F, 0.39F, 1.0F};
constexpr std::array<float, 4U> kColorClustered{0.92F, 0.33F, 0.91F, 1.0F};
constexpr std::array<float, 4U> kColorVehicleContour{0.84F, 0.84F, 0.87F, 1.0F};
constexpr std::array<float, 4U> kColorSensor{0.96F, 0.57F, 0.18F, 1.0F};
constexpr std::array<float, 4U> kColorStatic{0.75F, 0.75F, 0.92F, 1.0F};
constexpr std::array<float, 4U> kColorDynamic{0.99F, 0.48F, 0.22F, 1.0F};
constexpr std::array<float, 4U> kColorLineMarks{0.90F, 0.90F, 0.40F, 1.0F};

struct SensorPose {
    double x_m{0.0};
    double y_m{0.0};
    double mounting_deg{0.0};
    double fov_deg{100.0};
};

constexpr std::array<SensorPose, 12U> kDefaultSensors{
    SensorPose{3.238, 0.913, 87.0, 60.0},  SensorPose{3.6, 0.715, 38.0, 100.0},
    SensorPose{3.804, 0.276, 7.0, 100.0},  SensorPose{3.804, -0.276, -4.0, 75.0},
    SensorPose{3.6, -0.715, -28.0, 75.0},  SensorPose{3.238, -0.913, -87.0, 45.0},
    SensorPose{-0.775, -0.822, -100.0, 75.0}, SensorPose{-0.956, -0.71, -165.0, 75.0},
    SensorPose{-1.09, -0.25, -175.0, 75.0},  SensorPose{-1.09, 0.25, 173.0, 100.0},
    SensorPose{-0.956, 0.71, 151.0, 100.0},  SensorPose{-0.775, 0.822, 99.0, 100.0},
};

// LiDARProcessor parity: input coordinates are treated as [longitude, latitude] from legacy data,
// then mapped to visualizer VCS axes: X=latitude (lateral), Y=longitude (longitudinal).
ImVec2 world_to_screen(const ImVec2& center, float meters_to_pixels, double x_lon_m, double y_lat_m) {
    return {center.x - static_cast<float>(y_lat_m) * meters_to_pixels,
            center.y - static_cast<float>(x_lon_m) * meters_to_pixels};
}

void draw_vehicle(ImDrawList* draw_list, const ImVec2& center, float yaw_rad, float scale) {
    const float c = std::cos(yaw_rad);
    const float s = std::sin(yaw_rad);

    const ImVec2 p0{center.x + (c * 0.6F - s * 0.0F) * scale, center.y - (s * 0.6F + c * 0.0F) * scale};
    const ImVec2 p1{center.x + (c * -0.4F - s * -0.35F) * scale, center.y - (s * -0.4F + c * -0.35F) * scale};
    const ImVec2 p2{center.x + (c * -0.4F - s * 0.35F) * scale, center.y - (s * -0.4F + c * 0.35F) * scale};

    draw_list->AddTriangleFilled(p0, p1, p2, IM_COL32(245, 245, 245, 255));
    draw_list->AddTriangle(p0, p1, p2, IM_COL32(20, 20, 20, 255), 1.5F);
}

ImU32 to_rgba(const std::array<float, 4U>& color) {
    return IM_COL32(static_cast<int>(color[0] * 255.0F),
                    static_cast<int>(color[1] * 255.0F),
                    static_cast<int>(color[2] * 255.0F),
                    static_cast<int>(color[3] * 255.0F));
}

std::array<double, 2U> rotate_point(double x, double y, double yaw_rad) {
    const double c = std::cos(yaw_rad);
    const double s = std::sin(yaw_rad);
    return {c * x - s * y, s * x + c * y};
}

void draw_vehicle_contour(ImDrawList* draw_list,
                          const ImVec2& center,
                          float meters_to_pixels,
                          float yaw_rad,
                          const std::vector<ContourPoint>& contour) {
    if (contour.size() < 2U) {
        return;
    }

    const ImU32 color = to_rgba(kColorVehicleContour);
    for (std::size_t i = 0; i < contour.size(); ++i) {
        const auto& a = contour[i];
        const auto& b = contour[(i + 1U) % contour.size()];
        const auto ar = rotate_point(a.x_m, a.y_m, yaw_rad);
        const auto br = rotate_point(b.x_m, b.y_m, yaw_rad);
        const ImVec2 pa = world_to_screen(center, meters_to_pixels, ar[0], ar[1]);
        const ImVec2 pb = world_to_screen(center, meters_to_pixels, br[0], br[1]);
        draw_list->AddLine(pa, pb, color, 2.0F);
    }
}

void draw_sensors(ImDrawList* draw_list,
                  const ImVec2& center,
                  float meters_to_pixels,
                  float yaw_rad,
                  const std::vector<SensorCalibration>& sensors) {
    const ImU32 sensor_color = to_rgba(kColorSensor);
    const ImU32 ray_color = IM_COL32(220, 200, 112, 180);
    for (std::size_t i = 0; i < sensors.size(); ++i) {
        const auto& sensor = sensors[i];
        const auto sr = rotate_point(sensor.x_m, sensor.y_m, yaw_rad);
        const ImVec2 p = world_to_screen(center, meters_to_pixels, sr[0], sr[1]);
        draw_list->AddCircleFilled(p, 4.0F, sensor_color, 12);

        const double heading = static_cast<double>(yaw_rad) + static_cast<double>(sensor.mounting_deg) *
                                                            (std::numbers::pi_v<double> / 180.0);
        const double range_m = 0.35;
        const ImVec2 tip = world_to_screen(
            center, meters_to_pixels, sr[0] + std::cos(heading) * range_m, sr[1] + std::sin(heading) * range_m);
        draw_list->AddLine(p, tip, ray_color, 1.5F);

        const std::string label = std::to_string(i);
        const ImVec2 text_pos{p.x + 6.0F, p.y - 8.0F};
        draw_list->AddText(ImVec2{text_pos.x + 1.0F, text_pos.y + 1.0F}, IM_COL32(15, 15, 15, 220), label.c_str());
        draw_list->AddText(text_pos, IM_COL32(245, 245, 245, 255), label.c_str());
    }
}

bool map_signal_way_to_sensor_pair(std::uint8_t group_id, std::uint8_t signal_way_id, int& tx, int& rx) {
    const int base = (group_id == 1U) ? 6 : 0;
    if (group_id > 1U || signal_way_id > 15U) {
        return false;
    }

    switch (signal_way_id) {
        case 0: tx = base + 0; rx = base + 0; return true;
        case 1: tx = base + 0; rx = base + 1; return true;
        case 2: tx = base + 1; rx = base + 0; return true;
        case 3: tx = base + 1; rx = base + 1; return true;
        case 4: tx = base + 1; rx = base + 2; return true;
        case 5: tx = base + 2; rx = base + 1; return true;
        case 6: tx = base + 2; rx = base + 2; return true;
        case 7: tx = base + 2; rx = base + 3; return true;
        case 8: tx = base + 3; rx = base + 2; return true;
        case 9: tx = base + 3; rx = base + 3; return true;
        case 10: tx = base + 3; rx = base + 4; return true;
        case 11: tx = base + 4; rx = base + 3; return true;
        case 12: tx = base + 4; rx = base + 4; return true;
        case 13: tx = base + 4; rx = base + 5; return true;
        case 14: tx = base + 5; rx = base + 4; return true;
        case 15: tx = base + 5; rx = base + 5; return true;
        default: return false;
    }
}

void draw_ellipse_curves(ImDrawList* draw_list,
                         const ImVec2& center,
                         float meters_to_pixels,
                         const std::vector<SignalWay>& signal_ways,
                         bool front_only) {
    const ImU32 color = IM_COL32(120, 230, 120, 180);
    constexpr int kSegments = 64;
    for (const auto& sw : signal_ways) {
        int tx = 0;
        int rx = 0;
        if (!map_signal_way_to_sensor_pair(sw.group_id, sw.signal_way_id, tx, rx)) {
            continue;
        }
        if (tx < 0 || rx < 0 || tx >= static_cast<int>(kDefaultSensors.size()) || rx >= static_cast<int>(kDefaultSensors.size())) {
            continue;
        }
        if (front_only && (tx >= 6 || rx >= 6)) {
            continue;
        }

        const auto& s0 = kDefaultSensors[tx];
        const auto& s1 = kDefaultSensors[rx];
        const double distance = static_cast<double>(sw.distance_m);
        if (distance <= 0.0) {
            continue;
        }

        const double dx = s1.x_m - s0.x_m;
        const double dy = s1.y_m - s0.y_m;
        const double baseline = std::sqrt(dx * dx + dy * dy);
        if (distance <= 0.5 * baseline) {
            continue;
        }
        const double cx = 0.5 * (s0.x_m + s1.x_m);
        const double cy = 0.5 * (s0.y_m + s1.y_m);
        const double a = distance;
        const double b = std::sqrt(std::max(0.0, distance * distance - 0.25 * baseline * baseline));
        const double theta = std::atan2(dy, dx);
        const double ct = std::cos(theta);
        const double st = std::sin(theta);

        ImVec2 prev{};
        bool has_prev = false;
        for (int i = 0; i <= kSegments; ++i) {
            const double t = (static_cast<double>(i) / static_cast<double>(kSegments)) * (2.0 * std::numbers::pi_v<double>);
            const double ex = a * std::cos(t);
            const double ey = b * std::sin(t);
            const double x = cx + ex * ct - ey * st;
            const double y = cy + ex * st + ey * ct;
            const ImVec2 p = world_to_screen(center, meters_to_pixels, x, y);
            if (has_prev) {
                draw_list->AddLine(prev, p, color, 1.0F);
            }
            prev = p;
            has_prev = true;
        }
    }
}

std::vector<SensorCalibration> resolve_sensors_for_render(const VisualizerSettings& settings) {
    if (!settings.vehicle_geometry.sensors.empty()) {
        return settings.vehicle_geometry.sensors;
    }

    std::vector<SensorCalibration> sensors;
    sensors.reserve(kDefaultSensors.size());
    for (const auto& s : kDefaultSensors) {
        SensorCalibration c;
        c.x_m = static_cast<float>(s.x_m);
        c.y_m = static_cast<float>(s.y_m);
        c.mounting_deg = static_cast<float>(s.mounting_deg);
        c.fov_deg = static_cast<float>(s.fov_deg);
        sensors.push_back(c);
    }
    return sensors;
}

void draw_fov_pies(ImDrawList* draw_list,
                   const ImVec2& center,
                   float meters_to_pixels,
                   float yaw_rad,
                   const std::vector<SignalWay>& signal_ways,
                   const std::vector<SensorCalibration>& sensors,
                   bool front_only) {
    const ImU32 color = IM_COL32(230, 140, 210, 110);
    const ImU32 edge = IM_COL32(255, 170, 240, 170);
    constexpr int kSegments = 18;
    constexpr std::size_t kMaxDraw = 64U;

    const std::size_t draw_count = std::min(kMaxDraw, signal_ways.size());
    for (std::size_t i = 0; i < draw_count; ++i) {
        const auto& sw = signal_ways[i];
        int tx = 0;
        int rx = 0;
        if (!map_signal_way_to_sensor_pair(sw.group_id, sw.signal_way_id, tx, rx)) {
            continue;
        }
        if (tx < 0 || tx >= static_cast<int>(sensors.size()) || rx < 0 || rx >= static_cast<int>(sensors.size())) {
            continue;
        }
        if (front_only && (tx >= 6 || rx >= 6)) {
            continue;
        }
        const float range = sw.distance_m;
        if (range <= 0.0F) {
            continue;
        }

        auto draw_sector = [&](const SensorCalibration& s) {
            const auto sr = rotate_point(s.x_m, s.y_m, yaw_rad);
            const ImVec2 p0 = world_to_screen(center, meters_to_pixels, sr[0], sr[1]);
            const double base = static_cast<double>(yaw_rad) +
                                static_cast<double>(s.mounting_deg) * (std::numbers::pi_v<double> / 180.0);
            const double half_fov = 0.5 * static_cast<double>(s.fov_deg) * (std::numbers::pi_v<double> / 180.0);

            std::vector<ImVec2> pts;
            pts.reserve(static_cast<std::size_t>(kSegments) + 2U);
            pts.push_back(p0);
            for (int j = 0; j <= kSegments; ++j) {
                const double a = base - half_fov + (2.0 * half_fov * static_cast<double>(j) / static_cast<double>(kSegments));
                const double x = sr[0] + static_cast<double>(range) * std::cos(a);
                const double y = sr[1] + static_cast<double>(range) * std::sin(a);
                pts.push_back(world_to_screen(center, meters_to_pixels, x, y));
            }

            draw_list->AddConvexPolyFilled(pts.data(), static_cast<int>(pts.size()), color);
            draw_list->AddPolyline(pts.data(), static_cast<int>(pts.size()), edge, ImDrawFlags_Closed, 1.0F);
        };

        draw_sector(sensors[tx]);
        if (tx != rx) {
            draw_sector(sensors[rx]);
        }
    }
}

void draw_signal_way_traces(ImDrawList* draw_list,
                            const ImVec2& center,
                            float meters_to_pixels,
                            const std::vector<SignalWay>& signal_ways,
                            const std::vector<SensorCalibration>& sensors,
                            bool front_only) {
    const ImU32 line_color = IM_COL32(210, 210, 210, 150);
    constexpr std::size_t kMaxDraw = 80U;
    const std::size_t draw_count = std::min(kMaxDraw, signal_ways.size());
    for (std::size_t i = 0; i < draw_count; ++i) {
        const auto& sw = signal_ways[i];
        if (front_only && sw.group_id != 0U) {
            continue;
        }
        int tx = 0;
        int rx = 0;
        if (!map_signal_way_to_sensor_pair(sw.group_id, sw.signal_way_id, tx, rx)) {
            continue;
        }
        if (tx < 0 || tx >= static_cast<int>(sensors.size()) || rx < 0 || rx >= static_cast<int>(sensors.size())) {
            continue;
        }
        if (front_only && (tx >= 6 || rx >= 6)) {
            continue;
        }

        const auto& s0 = sensors[tx];
        const auto& s1 = sensors[rx];
        const double d = static_cast<double>(sw.distance_m);
        const double a0 = static_cast<double>(s0.mounting_deg) * (std::numbers::pi_v<double> / 180.0);
        const double a1 = static_cast<double>(s1.mounting_deg) * (std::numbers::pi_v<double> / 180.0);
        const double x0 = s0.x_m + static_cast<float>(d * std::cos(a0));
        const double y0 = s0.y_m + static_cast<float>(d * std::sin(a0));
        const double x1 = s1.x_m + static_cast<float>(d * std::cos(a1));
        const double y1 = s1.y_m + static_cast<float>(d * std::sin(a1));
        const double xc = 0.5 * (x0 + x1);
        const double yc = 0.5 * (y0 + y1);

        const ImVec2 ptx = world_to_screen(center, meters_to_pixels, s0.x_m, s0.y_m);
        const ImVec2 prx = world_to_screen(center, meters_to_pixels, s1.x_m, s1.y_m);
        const ImVec2 pdet = world_to_screen(center, meters_to_pixels, xc, yc);
        draw_list->AddLine(ptx, pdet, line_color, 1.2F);
        draw_list->AddLine(prx, pdet, line_color, 1.2F);
    }
}

void draw_static_features(ImDrawList* draw_list,
                          const ImVec2& center,
                          float meters_to_pixels,
                          const std::vector<StaticFeature>& features) {
    const ImU32 color = to_rgba(kColorStatic);
    for (const auto& f : features) {
        const ImVec2 p = world_to_screen(center, meters_to_pixels, f.x_m, f.y_m);
        draw_list->AddCircleFilled(p, 4.0F, color, 12);
    }
}

void draw_dynamic_features(ImDrawList* draw_list,
                           const ImVec2& center,
                           float meters_to_pixels,
                           const std::vector<DynamicFeature>& features) {
    const ImU32 color = to_rgba(kColorDynamic);
    for (const auto& f : features) {
        const ImVec2 p = world_to_screen(center, meters_to_pixels, f.x_m, f.y_m);
        const ImVec2 v = world_to_screen(center, meters_to_pixels, f.x_m + 0.5F * f.vx_mps, f.y_m + 0.5F * f.vy_mps);
        draw_list->AddCircleFilled(p, 4.5F, color, 12);
        draw_list->AddLine(p, v, color, 1.5F);
    }
}

void draw_line_marks(ImDrawList* draw_list,
                     const ImVec2& center,
                     float meters_to_pixels,
                     const std::vector<LineMark>& line_marks) {
    const ImU32 color = to_rgba(kColorLineMarks);
    for (const auto& lm : line_marks) {
        const ImVec2 p0 = world_to_screen(center, meters_to_pixels, lm.x0_m, lm.y0_m);
        const ImVec2 p1 = world_to_screen(center, meters_to_pixels, lm.x1_m, lm.y1_m);
        draw_list->AddLine(p0, p1, color, 2.0F);
    }
}

void draw_grid_map(ImDrawList* draw_list,
                   const ImVec2& center,
                   float meters_to_pixels,
                   const GridMap& grid) {
    if (!grid.valid || grid.rows == 0U || grid.cols == 0U || grid.occupancy.size() != (grid.rows * grid.cols)) {
        return;
    }
    for (std::uint32_t r = 0; r < grid.rows; ++r) {
        for (std::uint32_t c = 0; c < grid.cols; ++c) {
            const float occ = std::clamp(grid.occupancy[r * grid.cols + c], 0.0F, 1.0F);
            if (occ < 0.01F) {
                continue;
            }
            const float x0 = grid.origin_x_m + static_cast<float>(c) * grid.cell_size_m;
            const float y0 = grid.origin_y_m + static_cast<float>(r) * grid.cell_size_m;
            const float x1 = x0 + grid.cell_size_m;
            const float y1 = y0 + grid.cell_size_m;
            const ImVec2 p0 = world_to_screen(center, meters_to_pixels, x0, y0);
            const ImVec2 p1 = world_to_screen(center, meters_to_pixels, x1, y1);
            const int alpha = static_cast<int>(occ * 120.0F) + 20;
            draw_list->AddRectFilled(p0, p1, IM_COL32(80, 140, 190, alpha));
        }
    }
}

template <typename DetectionsT>
void draw_detections(ImDrawList*            draw_list,
                     const ImVec2&          center,
                     float                  meters_to_pixels,
                     const DetectionsT&     detections,
                     const std::array<float, 4U>& color,
                     float                  radius_px) {
    const ImU32 rgba = to_rgba(color);

    for (const auto& detection : detections) {
        const ImVec2 p = world_to_screen(center, meters_to_pixels, detection[0], detection[1]);
        draw_list->AddCircleFilled(p, radius_px, rgba, 16);
        draw_list->AddCircle(p, radius_px + 1.0F, IM_COL32(20, 20, 20, 210), 16, 1.0F);
    }
}

}  // namespace

int run_imgui_visualizer(const std::vector<FrameOutput>& frames, const VisualizerSettings& settings) {
    if (frames.empty()) {
        return 1;
    }

    if (!glfwInit()) {
        return 2;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1360, 860, "Ultrasound ImGui Visualizer", nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        return 3;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    std::size_t frame_index = 0U;
    bool autoplay = !settings.start_paused;
    bool loop_playback = settings.loop_playback;
    float playback_fps = settings.playback_fps;
    float meters_to_pixels = settings.meters_to_pixels;

    bool show_tracing = true;
    bool show_fov = true;
    bool show_ellipse = true;
    bool show_ellipse_curves = true;
    bool show_fused = true;
    bool show_clustered = true;
    bool show_vehicle_contour = settings.show_vehicle_contour;
    bool show_sensors = settings.show_sensors;
    bool show_static = true;
    bool show_dynamic = true;
    bool show_line_marks = true;
    bool show_gridmap = true;
    bool show_signal_way_traces = true;
    bool show_front_signal_way_traces = false;

    auto last_tick = std::chrono::steady_clock::now();
    double frame_accum_s = 0.0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        const auto now = std::chrono::steady_clock::now();
        frame_accum_s += std::chrono::duration<double>(now - last_tick).count();
        last_tick = now;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Playback");
        ImGui::Checkbox("Autoplay", &autoplay);
        ImGui::Checkbox("Loop", &loop_playback);
        ImGui::SliderFloat("FPS", &playback_fps, 1.0F, 60.0F, "%.1f");
        ImGui::SliderFloat("Scale (px/m)", &meters_to_pixels, 10.0F, 120.0F, "%.1f");
        ImGui::Separator();
        ImGui::Checkbox("Tracing", &show_tracing);
        ImGui::Checkbox("FOV", &show_fov);
        ImGui::Checkbox("Ellipse Detections", &show_ellipse);
        ImGui::Checkbox("Ellipse Models", &show_ellipse_curves);
        ImGui::Checkbox("Fused", &show_fused);
        ImGui::Checkbox("Clustered", &show_clustered);
        ImGui::Checkbox("Vehicle Contour", &show_vehicle_contour);
        ImGui::Checkbox("Sensors", &show_sensors);
        ImGui::Checkbox("Static Features", &show_static);
        ImGui::Checkbox("Dynamic Features", &show_dynamic);
        ImGui::Checkbox("Line Marks", &show_line_marks);
        ImGui::Checkbox("Gridmap", &show_gridmap);
        ImGui::Checkbox("SignalWay Traces", &show_signal_way_traces);
        ImGui::Checkbox("Front Only (Trace/FOV/EllipseModel)", &show_front_signal_way_traces);
        ImGui::Separator();

        int index_i = static_cast<int>(frame_index);
        const int max_index = static_cast<int>(frames.size() - 1U);
        if (ImGui::SliderInt("Frame", &index_i, 0, max_index)) {
            frame_index = static_cast<std::size_t>(std::clamp(index_i, 0, max_index));
        }

        if (autoplay && playback_fps > 0.0F) {
            const double period = 1.0 / static_cast<double>(playback_fps);
            while (frame_accum_s >= period) {
                frame_accum_s -= period;
                if (frame_index + 1U < frames.size()) {
                    ++frame_index;
                } else if (loop_playback) {
                    frame_index = 0U;
                } else {
                    autoplay = false;
                    break;
                }
            }
        } else {
            frame_accum_s = 0.0;
        }

        const auto& frame = frames.at(frame_index);
        const auto sensors_for_render = resolve_sensors_for_render(settings);
        ImGui::Text("timestamp_us: %llu", static_cast<unsigned long long>(frame.timestamp_us));
        ImGui::Text("pose [m,rad]: (%.3f, %.3f, %.3f)",
                    frame.observation_pose.x_m,
                    frame.observation_pose.y_m,
                    frame.observation_pose.yaw_rad);
        ImGui::Text("counts tracing=%zu fov=%zu ellipse=%zu fused=%zu",
                    frame.processed.tracing.size(),
                    frame.processed.fov_intersections.size(),
                    frame.processed.ellipse_intersections.size(),
                    frame.processed.fused.size());
        ImGui::Text("clustered=%zu", frame.processed.clustered.size());
        ImGui::Text("static=%zu dynamic=%zu lineMarks=%zu grid=%s",
                    frame.static_features.size(),
                    frame.dynamic_features.size(),
                    frame.line_marks.size(),
                    frame.grid_map.valid ? "yes" : "no");
        ImGui::TextUnformatted("Axes: X=Latitude (lateral), Y=Longitude (longitudinal)");
        ImGui::End();

        ImGui::Begin("Ultrasound Topdown");
        const bool topdown_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
        const ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
        const ImVec2 canvas_size = ImGui::GetContentRegionAvail();
        const ImVec2 canvas_p1{canvas_p0.x + canvas_size.x, canvas_p0.y + canvas_size.y};

        if (topdown_hovered && std::fabs(io.MouseWheel) > 1.0e-6F) {
            const float zoom_factor = std::pow(1.12F, io.MouseWheel);
            meters_to_pixels = std::clamp(meters_to_pixels * zoom_factor, 5.0F, 250.0F);
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(18, 20, 24, 255));
        draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(90, 95, 104, 255));

        const ImVec2 center{(canvas_p0.x + canvas_p1.x) * 0.5F, (canvas_p0.y + canvas_p1.y) * 0.5F};

        for (int m = -20; m <= 20; ++m) {
            const float x = center.x + static_cast<float>(m) * meters_to_pixels;
            const float y = center.y + static_cast<float>(m) * meters_to_pixels;
            const ImU32 grid = (m == 0) ? IM_COL32(140, 145, 155, 120) : IM_COL32(70, 74, 82, 70);
            draw_list->AddLine(ImVec2{x, canvas_p0.y}, ImVec2{x, canvas_p1.y}, grid);
            draw_list->AddLine(ImVec2{canvas_p0.x, y}, ImVec2{canvas_p1.x, y}, grid);
        }

        if (show_vehicle_contour && !settings.vehicle_geometry.contour.empty()) {
            draw_vehicle_contour(
                draw_list, center, meters_to_pixels, frame.observation_pose.yaw_rad, settings.vehicle_geometry.contour);
        } else {
            draw_vehicle(draw_list, center, frame.observation_pose.yaw_rad, meters_to_pixels);
        }

        if (show_sensors && !settings.vehicle_geometry.sensors.empty()) {
            draw_sensors(
                draw_list, center, meters_to_pixels, frame.observation_pose.yaw_rad, settings.vehicle_geometry.sensors);
        } else if (show_sensors) {
            draw_sensors(draw_list, center, meters_to_pixels, frame.observation_pose.yaw_rad, sensors_for_render);
        }

        if (show_gridmap) {
            draw_grid_map(draw_list, center, meters_to_pixels, frame.grid_map);
        }
        if (show_signal_way_traces) {
            draw_signal_way_traces(
                draw_list, center, meters_to_pixels, frame.signal_ways, sensors_for_render, show_front_signal_way_traces);
        }
        if (show_line_marks) {
            draw_line_marks(draw_list, center, meters_to_pixels, frame.line_marks);
        }
        if (show_static) {
            draw_static_features(draw_list, center, meters_to_pixels, frame.static_features);
        }
        if (show_dynamic) {
            draw_dynamic_features(draw_list, center, meters_to_pixels, frame.dynamic_features);
        }

        if (show_tracing) {
            draw_detections(draw_list, center, meters_to_pixels, frame.processed.tracing, kColorTracing, 6.0F);
        }
        if (show_fov) {
            draw_fov_pies(
                draw_list,
                center,
                meters_to_pixels,
                frame.observation_pose.yaw_rad,
                frame.signal_ways,
                sensors_for_render,
                show_front_signal_way_traces);
            draw_detections(draw_list, center, meters_to_pixels, frame.processed.fov_intersections, kColorFov, 6.0F);
        }
        if (show_ellipse_curves) {
            draw_ellipse_curves(draw_list, center, meters_to_pixels, frame.signal_ways, show_front_signal_way_traces);
        }
        if (show_ellipse) {
            draw_detections(
                draw_list, center, meters_to_pixels, frame.processed.ellipse_intersections, kColorEllipse, 6.0F);
        }
        if (show_fused) {
            draw_detections(draw_list, center, meters_to_pixels, frame.processed.fused, kColorFused, 8.0F);
        }
        if (show_clustered) {
            draw_detections(draw_list, center, meters_to_pixels, frame.processed.clustered, kColorClustered, 9.0F);
        }

        if (frame.processed.tracing.empty() && frame.processed.fov_intersections.empty() &&
            frame.processed.ellipse_intersections.empty()) {
            draw_list->AddText(
                ImVec2{canvas_p0.x + 12.0F, canvas_p0.y + 12.0F},
                IM_COL32(230, 230, 235, 255),
                "No method detections in this frame");
        }
        ImGui::End();

        ImGui::Render();
        int display_w = 0;
        int display_h = 0;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.09F, 0.09F, 0.10F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

}  // namespace ultrasound
