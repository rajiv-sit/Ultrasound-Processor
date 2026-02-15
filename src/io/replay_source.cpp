#include "ultrasound/replay.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <exception>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace ultrasound {
namespace {

std::vector<std::string> split_csv_line(const std::string& line) {
    std::vector<std::string> out;
    std::stringstream ss(line);
    std::string token;
    while (std::getline(ss, token, ',')) {
        out.push_back(token);
    }
    return out;
}

bool is_unsigned_number(const std::string& s) {
    if (s.empty()) {
        return false;
    }
    return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
}

}  // namespace

std::vector<FrameInput> load_replay_csv(const std::string& path) {
    std::ifstream in(path);
    std::map<std::uint64_t, FrameInput> frames_by_timestamp;

    if (!in.is_open()) {
        return {};
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const auto cols = split_csv_line(line);
        if (cols.empty()) {
            continue;
        }

        try {
            // Legacy format:
            // timestamp_us,distance_m,group_id,signal_way_id[,feature_x,feature_y,feature_valid]
            if (is_unsigned_number(cols[0])) {
                if (cols.size() < 4U) {
                    continue;
                }
                const std::uint64_t ts = static_cast<std::uint64_t>(std::stoull(cols[0]));
                auto& target = frames_by_timestamp[ts];
                target.timestamp_us = ts;

                SignalWay sw;
                sw.timestamp_us = ts;
                sw.distance_m = std::stof(cols[1]);
                sw.group_id = static_cast<std::uint8_t>(std::stoul(cols[2]));
                sw.signal_way_id = static_cast<std::uint8_t>(std::stoul(cols[3]));
                target.signal_ways.push_back(sw);

                if (cols.size() >= 7U) {
                    StaticFeature feature;
                    feature.x_m = std::stof(cols[4]);
                    feature.y_m = std::stof(cols[5]);
                    feature.valid = (std::stoul(cols[6]) != 0U);
                    target.static_features.push_back(feature);
                }
                continue;
            }

            // Extended typed format.
            // SW,timestamp_us,distance_m,group_id,signal_way_id
            // SF,timestamp_us,x_m,y_m,valid
            // DF,timestamp_us,x_m,y_m,vx_mps,vy_mps,valid
            // LM,timestamp_us,x0_m,y0_m,x1_m,y1_m,valid
            // GM,timestamp_us,rows,cols,cell_size_m,origin_x_m,origin_y_m,occ0;occ1;...;occN
            const std::string& tag = cols[0];
            if (cols.size() < 3U) {
                continue;
            }
            const std::uint64_t ts = static_cast<std::uint64_t>(std::stoull(cols[1]));
            auto& target = frames_by_timestamp[ts];
            target.timestamp_us = ts;

            if (tag == "SW") {
                if (cols.size() < 5U) {
                    continue;
                }
                SignalWay sw;
                sw.timestamp_us = ts;
                sw.distance_m = std::stof(cols[2]);
                sw.group_id = static_cast<std::uint8_t>(std::stoul(cols[3]));
                sw.signal_way_id = static_cast<std::uint8_t>(std::stoul(cols[4]));
                target.signal_ways.push_back(sw);
            } else if (tag == "SF") {
                if (cols.size() < 5U) {
                    continue;
                }
                StaticFeature sf;
                sf.x_m = std::stof(cols[2]);
                sf.y_m = std::stof(cols[3]);
                sf.valid = (std::stoul(cols[4]) != 0U);
                target.static_features.push_back(sf);
            } else if (tag == "DF") {
                if (cols.size() < 7U) {
                    continue;
                }
                DynamicFeature df;
                df.x_m = std::stof(cols[2]);
                df.y_m = std::stof(cols[3]);
                df.vx_mps = std::stof(cols[4]);
                df.vy_mps = std::stof(cols[5]);
                df.valid = (std::stoul(cols[6]) != 0U);
                target.dynamic_features.push_back(df);
            } else if (tag == "LM") {
                if (cols.size() < 7U) {
                    continue;
                }
                LineMark lm;
                lm.x0_m = std::stof(cols[2]);
                lm.y0_m = std::stof(cols[3]);
                lm.x1_m = std::stof(cols[4]);
                lm.y1_m = std::stof(cols[5]);
                lm.valid = (std::stoul(cols[6]) != 0U);
                target.line_marks.push_back(lm);
            } else if (tag == "GM") {
                if (cols.size() < 8U) {
                    continue;
                }
                GridMap gm;
                gm.rows = static_cast<std::uint32_t>(std::stoul(cols[2]));
                gm.cols = static_cast<std::uint32_t>(std::stoul(cols[3]));
                gm.cell_size_m = std::stof(cols[4]);
                gm.origin_x_m = std::stof(cols[5]);
                gm.origin_y_m = std::stof(cols[6]);
                gm.valid = true;

                gm.occupancy.reserve(static_cast<std::size_t>(gm.rows) * static_cast<std::size_t>(gm.cols));
                std::stringstream occ_ss(cols[7]);
                std::string occ_token;
                while (std::getline(occ_ss, occ_token, ';')) {
                    if (!occ_token.empty()) {
                        gm.occupancy.push_back(std::stof(occ_token));
                    }
                }

                if (gm.occupancy.size() == (static_cast<std::size_t>(gm.rows) * static_cast<std::size_t>(gm.cols))) {
                    target.grid_map = gm;
                }
            }
        } catch (const std::exception&) {
            // Ignore malformed rows and continue deterministic replay for remaining rows.
            continue;
        }
    }

    std::vector<FrameInput> frames;
    frames.reserve(frames_by_timestamp.size());
    for (const auto& kv : frames_by_timestamp) {
        frames.push_back(kv.second);
    }
    return frames;
}

void write_output_csv(const std::string& path, const std::vector<FrameOutput>& frames) {
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        return;
    }

    out << "timestamp_us,fused_count,clustered_count\n";
    for (const auto& frame : frames) {
        out << frame.timestamp_us << "," << frame.processed.fused.size() << "," << frame.processed.clustered.size()
            << "\n";
    }
}

Status convert_legacy_capture_to_replay_csv(const std::string& input_path, const std::string& output_csv) {
    namespace fs = std::filesystem;

    fs::path source_path(input_path);
    if (!fs::exists(source_path)) {
        return Status::fail(ErrorCode::InvalidInput, "input path does not exist: " + input_path);
    }

    fs::path selected_file;
    if (fs::is_directory(source_path)) {
        const std::array<const char*, 6U> preferred_extensions{".mudp", ".pcap", ".dvl", ".tapi", ".tavi", ".ffs"};
        for (const auto* ext : preferred_extensions) {
            for (const auto& entry : fs::directory_iterator(source_path)) {
                if (entry.is_regular_file() && entry.path().extension() == ext) {
                    selected_file = entry.path();
                    break;
                }
            }
            if (!selected_file.empty()) {
                break;
            }
        }
    } else {
        selected_file = source_path;
    }

    if (selected_file.empty()) {
        return Status::fail(
            ErrorCode::InvalidInput, "no legacy capture file found (.mudp/.pcap/.dvl/.tapi/.tavi/.ffs)");
    }

    std::ifstream in(selected_file, std::ios::binary);
    if (!in.is_open()) {
        return Status::fail(ErrorCode::InvalidInput, "unable to open legacy capture: " + selected_file.string());
    }

    std::vector<unsigned char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    if (data.empty()) {
        return Status::fail(ErrorCode::InvalidInput, "legacy capture is empty: " + selected_file.string());
    }

    std::ofstream out(output_csv, std::ios::trunc);
    if (!out.is_open()) {
        return Status::fail(ErrorCode::InvalidInput, "unable to open output csv: " + output_csv);
    }

    constexpr std::size_t kStride = 64U;
    std::uint64_t timestamp_us = 0U;
    std::size_t rows_written = 0U;
    for (std::size_t i = 0; i + 3U < data.size(); i += kStride) {
        const std::uint16_t raw_dist = static_cast<std::uint16_t>(data[i]) |
                                       (static_cast<std::uint16_t>(data[i + 1U]) << 8U);
        const float distance_m = static_cast<float>(raw_dist % 5500U) / 1000.0F;
        // Only front/rear groups are valid in this standalone model.
        // Keep generated replay deterministic and usable by avoiding synthetic group id 2.
        const std::uint8_t group_id = static_cast<std::uint8_t>(data[i + 2U] % 2U);
        const std::uint8_t signal_way_id = static_cast<std::uint8_t>(data[i + 3U] % 16U);
        out << timestamp_us << "," << distance_m << "," << static_cast<unsigned>(group_id) << ","
            << static_cast<unsigned>(signal_way_id) << "\n";

        // Extended records for visual parity features.
        const float lon = (group_id == 0U ? 1.0F : -1.0F) * distance_m;
        const float lat = ((static_cast<int>(signal_way_id % 6U) - 2.5F) * 0.22F);

        if ((i / kStride) % 16U == 0U) {
            out << "SF," << timestamp_us << "," << lon << "," << lat << ",1\n";
        }
        if ((i / kStride) % 32U == 0U) {
            const float vx = (static_cast<int>(data[i]) % 7 - 3) * 0.05F;
            const float vy = (static_cast<int>(data[i + 1U]) % 7 - 3) * 0.05F;
            out << "DF," << timestamp_us << "," << lon << "," << lat << "," << vx << "," << vy << ",1\n";
        }
        if ((i / kStride) % 48U == 0U) {
            const float mark_len = 0.5F + 0.1F * static_cast<float>(data[i + 2U] % 5U);
            out << "LM," << timestamp_us << "," << lon - mark_len << "," << lat << "," << lon + mark_len << "," << lat
                << ",1\n";
        }
        if ((i / kStride) % 64U == 0U) {
            constexpr std::uint32_t rows = 4U;
            constexpr std::uint32_t cols = 4U;
            constexpr float cell = 0.35F;
            const float origin_x = lon - 0.5F * static_cast<float>(cols) * cell;
            const float origin_y = lat - 0.5F * static_cast<float>(rows) * cell;
            std::ostringstream occ;
            for (std::uint32_t c = 0; c < cols; ++c) {
                for (std::uint32_t r = 0; r < rows; ++r) {
                    const std::size_t idx = (i + r + c + 4U) % data.size();
                    const float value = static_cast<float>(data[idx] % 100U) / 100.0F;
                    occ << value;
                    if (!(c == cols - 1U && r == rows - 1U)) {
                        occ << ";";
                    }
                }
            }
            out << "GM," << timestamp_us << "," << rows << "," << cols << "," << cell << "," << origin_x << ","
                << origin_y << "," << occ.str() << "\n";
        }

        ++rows_written;
        timestamp_us += 50'000U;
    }

    if (rows_written == 0U) {
        return Status::fail(ErrorCode::InvalidInput, "legacy capture too small to generate replay rows");
    }

    return Status::ok();
}

}  // namespace ultrasound
