#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "ultrasound/config_io.hpp"
#include "ultrasound/vehicle_geometry.hpp"

namespace {

std::filesystem::path temp_file(const std::string& name) {
    return std::filesystem::temp_directory_path() / name;
}

TEST(ConfigLoaderTest, LoadsProcessorConfigFromIni) {
    const auto path = temp_file("uss_cfg_ok.ini");
    {
        std::ofstream out(path, std::ios::trunc);
        out << "[General]\n";
        out << "minRangeM=0.1\n";
        out << "maxRangeM=6.2\n";
        out << "strictMonotonicTimestamps=false\n";
        out << "[Conversion]\n";
        out << "nSigmaValeo=4.5\n";
        out << "legacyValeoBugfix=true\n";
        out << "[SignalWays]\n";
        out << "groupID=REAR\n";
        out << "method=FOV_INTERSECTION\n";
        out << "clusterRadiusM=0.7\n";
    }

    ultrasound::ProcessorConfig cfg;
    const auto st = ultrasound::load_processor_config_from_ini(path.string(), cfg);
    std::filesystem::remove(path);

    ASSERT_TRUE(st.is_ok());
    EXPECT_FLOAT_EQ(cfg.n_sigma_valeo, 4.5F);
    EXPECT_TRUE(cfg.use_legacy_valeo_bugfix);
    EXPECT_EQ(cfg.group_filter, ultrasound::GroupFilter::Rear);
    EXPECT_EQ(cfg.processing_method, ultrasound::ProcessingMethod::FovIntersection);
    EXPECT_FLOAT_EQ(cfg.min_range_m, 0.1F);
    EXPECT_FLOAT_EQ(cfg.max_range_m, 6.2F);
    EXPECT_FLOAT_EQ(cfg.cluster_radius_m, 0.7F);
    EXPECT_FALSE(cfg.strict_monotonic_timestamps);
}

TEST(ConfigLoaderTest, RejectsInvalidProcessorConfig) {
    const auto path = temp_file("uss_cfg_bad.ini");
    {
        std::ofstream out(path, std::ios::trunc);
        out << "[General]\n";
        out << "minRangeM=3.0\n";
        out << "maxRangeM=2.0\n";
        out << "strictMonotonicTimestamps=maybe\n";
    }

    ultrasound::ProcessorConfig cfg;
    const auto st = ultrasound::load_processor_config_from_ini(path.string(), cfg);
    std::filesystem::remove(path);

    EXPECT_FALSE(st.is_ok());
    EXPECT_EQ(st.code, ultrasound::ErrorCode::InvalidInput);
}

TEST(ConfigLoaderTest, RejectsMissingFile) {
    ultrasound::ProcessorConfig cfg;
    const auto st = ultrasound::load_processor_config_from_ini("C:/definitely_missing_uss_cfg.ini", cfg);
    EXPECT_FALSE(st.is_ok());
    EXPECT_EQ(st.code, ultrasound::ErrorCode::InvalidInput);
}

TEST(ConfigLoaderTest, LoadsVehicleGeometryFromIni) {
    const auto path = temp_file("uss_vehicle_ok.ini");
    {
        std::ofstream out(path, std::ios::trunc);
        out << "[Contour]\n";
        out << "contourPt0=-1.0,0.5\n";
        out << "contourPt1=-1.0,-0.5\n";
        out << "contourPt2=2.0,-0.5\n";
        out << "contourPt3=2.0,0.5\n";
        out << "[USS SENSORS]\n";
        out << "uss_position_0=2.0,0.3\n";
        out << "uss_mounting_0=10.0,80.0\n";
        out << "uss_position_1=2.0,-0.3\n";
        out << "uss_mounting_1=-10.0,80.0\n";
    }

    ultrasound::VehicleGeometry geometry;
    const auto st = ultrasound::load_vehicle_geometry_from_ini(path.string(), geometry);
    std::filesystem::remove(path);

    ASSERT_TRUE(st.is_ok());
    ASSERT_EQ(geometry.contour.size(), 4U);
    ASSERT_EQ(geometry.sensors.size(), 2U);
    EXPECT_FLOAT_EQ(geometry.sensors[0].x_m, 2.0F);
    EXPECT_FLOAT_EQ(geometry.sensors[0].fov_deg, 80.0F);
}

TEST(ConfigLoaderTest, RejectsInvalidVehicleGeometry) {
    const auto path = temp_file("uss_vehicle_bad.ini");
    {
        std::ofstream out(path, std::ios::trunc);
        out << "[Contour]\n";
        out << "contourPt0=-1.0\n";
        out << "[USS SENSORS]\n";
        out << "uss_position_0=2.0,0.3\n";
    }

    ultrasound::VehicleGeometry geometry;
    const auto st = ultrasound::load_vehicle_geometry_from_ini(path.string(), geometry);
    std::filesystem::remove(path);

    EXPECT_FALSE(st.is_ok());
    EXPECT_EQ(st.code, ultrasound::ErrorCode::InvalidInput);
}

}  // namespace
