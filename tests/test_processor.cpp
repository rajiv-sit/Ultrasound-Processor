#include <cmath>

#include <gtest/gtest.h>

#include "ultrasound/processor.hpp"

namespace {

using ultrasound::ErrorCode;
using ultrasound::FrameInput;
using ultrasound::GroupFilter;
using ultrasound::ProcessingMethod;
using ultrasound::ProcessorConfig;
using ultrasound::SignalWay;
using ultrasound::UltrasoundProcessor;
using ultrasound::VehicleState;

void seed_states(UltrasoundProcessor& p) {
    VehicleState s0;
    s0.timestamp_us = 1000U;
    s0.pose.x_m = 1.0F;
    s0.pose.y_m = 0.0F;
    s0.pose.yaw_rad = 0.0F;
    ASSERT_TRUE(p.push_vehicle_state(s0).is_ok());

    VehicleState s1;
    s1.timestamp_us = 2000U;
    s1.pose.x_m = 3.0F;
    s1.pose.y_m = 2.0F;
    s1.pose.yaw_rad = 0.4F;
    ASSERT_TRUE(p.push_vehicle_state(s1).is_ok());
}

TEST(ProcessorTest, PushVehicleStateRejectsNonMonotonicTimestamps) {
    UltrasoundProcessor p;
    VehicleState s0;
    s0.timestamp_us = 5000U;
    ASSERT_TRUE(p.push_vehicle_state(s0).is_ok());

    VehicleState s1;
    s1.timestamp_us = 5000U;
    const auto st = p.push_vehicle_state(s1);
    EXPECT_FALSE(st.is_ok());
    EXPECT_EQ(st.code, ErrorCode::InvalidInput);
}

TEST(ProcessorTest, ProcessFrameRequiresVehicleState) {
    UltrasoundProcessor p;
    FrameInput in;
    in.timestamp_us = 1500U;
    in.signal_ways.push_back({1500U, 1.2F, 0U, 1U});

    const auto st = p.process_frame(in);
    EXPECT_FALSE(st.is_ok());
    EXPECT_EQ(st.code, ErrorCode::MissingVehicleState);
}

TEST(ProcessorTest, ProcessFrameRejectsEmptyInput) {
    UltrasoundProcessor p;
    seed_states(p);

    FrameInput in;
    in.timestamp_us = 1500U;

    const auto st = p.process_frame(in);
    EXPECT_FALSE(st.is_ok());
    EXPECT_EQ(st.code, ErrorCode::InvalidInput);
}

TEST(ProcessorTest, ProcessFrameInterpolatesPoseAndFiltersSignalWays) {
    ProcessorConfig cfg;
    cfg.group_filter = GroupFilter::Front;
    cfg.min_range_m = 0.5F;
    cfg.max_range_m = 3.0F;
    cfg.processing_method = ProcessingMethod::SignalTracing;

    UltrasoundProcessor p(cfg);
    seed_states(p);

    FrameInput in;
    in.timestamp_us = 1500U;
    in.signal_ways.push_back({1500U, 2.0F, 0U, 1U});  // kept
    in.signal_ways.push_back({1500U, 0.1F, 0U, 2U});  // filtered by min range
    in.signal_ways.push_back({1500U, 2.5F, 1U, 3U});  // filtered by group
    ultrasound::StaticFeature sf_ok;
    sf_ok.x_m = 1.0F;
    sf_ok.y_m = 2.0F;
    sf_ok.valid = true;
    in.static_features.push_back(sf_ok);

    ultrasound::StaticFeature sf_bad;
    sf_bad.valid = false;
    in.static_features.push_back(sf_bad);

    ultrasound::DynamicFeature df_ok;
    df_ok.x_m = 1.0F;
    df_ok.y_m = 1.0F;
    df_ok.valid = true;
    in.dynamic_features.push_back(df_ok);

    ultrasound::LineMark lm_ok;
    lm_ok.x0_m = 0.0F;
    lm_ok.y0_m = 0.0F;
    lm_ok.x1_m = 1.0F;
    lm_ok.y1_m = 1.0F;
    lm_ok.valid = true;
    in.line_marks.push_back(lm_ok);
    in.grid_map.valid = true;
    in.grid_map.rows = 2U;
    in.grid_map.cols = 2U;
    in.grid_map.occupancy = {0.1F, 0.2F, 0.3F, 0.4F};

    ASSERT_TRUE(p.process_frame(in).is_ok());
    const auto out = p.last_output();
    ASSERT_TRUE(out.has_value());
    EXPECT_EQ(out->signal_ways.size(), 1U);
    EXPECT_EQ(out->static_features.size(), 1U);
    EXPECT_EQ(out->dynamic_features.size(), 1U);
    EXPECT_EQ(out->line_marks.size(), 1U);
    EXPECT_TRUE(out->grid_map.valid);
    EXPECT_NEAR(out->observation_pose.x_m, 2.0F, 1e-6F);
    EXPECT_NEAR(out->observation_pose.y_m, 1.0F, 1e-6F);
    EXPECT_FALSE(out->processed.tracing.empty());
}

TEST(ProcessorTest, MonotonicGuardCanBeDisabled) {
    ProcessorConfig cfg;
    cfg.strict_monotonic_timestamps = false;
    UltrasoundProcessor p(cfg);
    seed_states(p);

    FrameInput f0;
    f0.timestamp_us = 1500U;
    f0.signal_ways.push_back({1500U, 1.0F, 0U, 1U});
    ASSERT_TRUE(p.process_frame(f0).is_ok());

    FrameInput f1;
    f1.timestamp_us = 1400U;
    f1.signal_ways.push_back({1400U, 1.1F, 0U, 2U});
    EXPECT_TRUE(p.process_frame(f1).is_ok());
}

TEST(ProcessorTest, AllMethodsProduceDetectionsAndDiagnosticsUpdate) {
    ProcessorConfig cfg;
    cfg.processing_method = ProcessingMethod::All;
    cfg.group_filter = GroupFilter::Surround;
    cfg.cluster_radius_m = 0.5F;

    UltrasoundProcessor p(cfg);
    seed_states(p);

    FrameInput in;
    in.timestamp_us = 1500U;
    in.signal_ways.push_back({1500U, 2.0F, 0U, 1U});
    in.signal_ways.push_back({1500U, 2.1F, 0U, 2U});
    in.signal_ways.push_back({1500U, 2.3F, 1U, 13U});
    in.signal_ways.push_back({1500U, 2.4F, 1U, 14U});

    ASSERT_TRUE(p.process_frame(in).is_ok());
    const auto out = p.last_output();
    ASSERT_TRUE(out.has_value());
    EXPECT_FALSE(out->processed.tracing.empty());
    EXPECT_FALSE(out->processed.fov_intersections.empty());
    EXPECT_FALSE(out->processed.ellipse_intersections.empty());
    EXPECT_FALSE(out->processed.fused.empty());
    EXPECT_FALSE(out->processed.clustered.empty());

    const auto d = p.diagnostics();
    EXPECT_EQ(d.processed_frames, 1U);
    EXPECT_GE(d.clustered_detections, out->processed.clustered.size());
}

TEST(ProcessorTest, DeterministicOutputForSameInputs) {
    ProcessorConfig cfg;
    cfg.processing_method = ProcessingMethod::All;

    UltrasoundProcessor p0(cfg);
    UltrasoundProcessor p1(cfg);
    seed_states(p0);
    seed_states(p1);

    FrameInput in;
    in.timestamp_us = 1500U;
    in.signal_ways.push_back({1500U, 2.0F, 0U, 1U});
    in.signal_ways.push_back({1500U, 2.5F, 1U, 13U});

    ASSERT_TRUE(p0.process_frame(in).is_ok());
    ASSERT_TRUE(p1.process_frame(in).is_ok());
    const auto o0 = p0.last_output();
    const auto o1 = p1.last_output();
    ASSERT_TRUE(o0.has_value());
    ASSERT_TRUE(o1.has_value());
    EXPECT_EQ(o0->processed.fused, o1->processed.fused);
    EXPECT_EQ(o0->processed.clustered, o1->processed.clustered);
}

}  // namespace
