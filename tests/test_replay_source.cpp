#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "ultrasound/replay.hpp"

namespace {

std::filesystem::path temp_path(const std::string& file) {
    return std::filesystem::temp_directory_path() / file;
}

TEST(ReplaySourceTest, LoadsLegacyAndTypedRowsIntoFrames) {
    const auto in_path = temp_path("uss_replay_mix.csv");
    {
        std::ofstream out(in_path, std::ios::trunc);
        out << "1000,1.0,0,1\n";
        out << "1000,2.0,1,2\n";
        out << "SW,1100,1.5,0,3\n";
        out << "SF,1100,1.2,0.3,1\n";
        out << "DF,1100,1.2,0.3,0.1,0.0,1\n";
        out << "LM,1100,0.0,0.0,1.0,0.0,1\n";
        out << "GM,1100,2,2,0.5,0.0,0.0,0.1;0.2;0.3;0.4\n";
        out << "MALFORMED,ROW\n";
    }

    const auto frames = ultrasound::load_replay_csv(in_path.string());
    std::filesystem::remove(in_path);

    ASSERT_EQ(frames.size(), 2U);
    EXPECT_EQ(frames[0].timestamp_us, 1000U);
    EXPECT_EQ(frames[0].signal_ways.size(), 2U);
    EXPECT_EQ(frames[1].timestamp_us, 1100U);
    EXPECT_EQ(frames[1].signal_ways.size(), 1U);
    EXPECT_EQ(frames[1].static_features.size(), 1U);
    EXPECT_EQ(frames[1].dynamic_features.size(), 1U);
    EXPECT_EQ(frames[1].line_marks.size(), 1U);
    EXPECT_TRUE(frames[1].grid_map.valid);
    EXPECT_EQ(frames[1].grid_map.occupancy.size(), 4U);
}

TEST(ReplaySourceTest, WritesOutputCsv) {
    const auto out_path = temp_path("uss_output.csv");
    ultrasound::FrameOutput frame;
    frame.timestamp_us = 1234U;
    frame.processed.fused.push_back({1.0, 1.0});
    frame.processed.clustered.push_back({1.2, 0.8});

    ultrasound::write_output_csv(out_path.string(), {frame});

    std::ifstream in(out_path);
    ASSERT_TRUE(in.is_open());
    std::string header;
    std::string row;
    std::getline(in, header);
    std::getline(in, row);
    in.close();
    std::filesystem::remove(out_path);

    EXPECT_EQ(header, "timestamp_us,fused_count,clustered_count");
    EXPECT_EQ(row, "1234,1,1");
}

TEST(ReplaySourceTest, LegacyConverterHandlesInvalidInputPaths) {
    const auto st = ultrasound::convert_legacy_capture_to_replay_csv(
        "C:/definitely_missing_legacy_capture.mudp",
        temp_path("not_written.csv").string());
    EXPECT_FALSE(st.is_ok());
    EXPECT_EQ(st.code, ultrasound::ErrorCode::InvalidInput);
}

TEST(ReplaySourceTest, LegacyConverterConvertsBinaryCapture) {
    const auto bin_path = temp_path("uss_legacy.bin");
    const auto csv_path = temp_path("uss_converted.csv");
    {
        std::ofstream out(bin_path, std::ios::binary | std::ios::trunc);
        for (int i = 0; i < 2048; ++i) {
            const unsigned char value = static_cast<unsigned char>(i % 251);
            out.write(reinterpret_cast<const char*>(&value), 1);
        }
    }

    const auto st = ultrasound::convert_legacy_capture_to_replay_csv(bin_path.string(), csv_path.string());
    ASSERT_TRUE(st.is_ok());

    const auto frames = ultrasound::load_replay_csv(csv_path.string());
    EXPECT_FALSE(frames.empty());

    std::filesystem::remove(bin_path);
    std::filesystem::remove(csv_path);
}

TEST(ReplaySourceTest, LegacyConverterDirectoryWithoutCaptureFails) {
    const auto dir = temp_path("uss_empty_legacy_dir");
    std::filesystem::create_directories(dir);

    const auto st = ultrasound::convert_legacy_capture_to_replay_csv(dir.string(), temp_path("unused.csv").string());
    EXPECT_FALSE(st.is_ok());
    EXPECT_EQ(st.code, ultrasound::ErrorCode::InvalidInput);

    std::filesystem::remove_all(dir);
}

}  // namespace
