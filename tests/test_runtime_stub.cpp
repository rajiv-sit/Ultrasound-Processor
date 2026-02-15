#include <gtest/gtest.h>

#include "ultrasound/runtime.hpp"

namespace {

TEST(RuntimeStubTest, ReportsAvailableRuntimeAdapter) {
    const auto status = ultrasound::query_runtime_adapter();
    EXPECT_TRUE(status.available);
    EXPECT_FALSE(status.description.empty());
}

TEST(RuntimeStubTest, DispatchInvokesRegisteredCallbacks) {
    ultrasound::clear_runtime_callbacks();

    bool sw_called = false;
    bool sf_called = false;
    bool df_called = false;
    bool lm_called = false;
    bool gm_called = false;
    bool pd_called = false;

    ultrasound::register_signal_ways_callback([&](const std::vector<ultrasound::SignalWay>& sw, std::uint64_t ts) {
        sw_called = (!sw.empty() && ts == 1000U);
    });
    ultrasound::register_static_features_callback([&](const std::vector<ultrasound::StaticFeature>& sf, std::uint64_t ts) {
        sf_called = (!sf.empty() && ts == 1000U);
    });
    ultrasound::register_dynamic_features_callback([&](const std::vector<ultrasound::DynamicFeature>& df, std::uint64_t ts) {
        df_called = (!df.empty() && ts == 1000U);
    });
    ultrasound::register_line_marks_callback([&](const std::vector<ultrasound::LineMark>& lm, std::uint64_t ts) {
        lm_called = (!lm.empty() && ts == 1000U);
    });
    ultrasound::register_grid_map_callback([&](const ultrasound::GridMap& gm, std::uint64_t ts) {
        gm_called = (gm.valid && ts == 1000U);
    });
    ultrasound::register_processed_detections_callback([&](const ultrasound::ProcessedDetections& pd, std::uint64_t ts) {
        pd_called = (!pd.fused.empty() && ts == 1000U);
    });

    ultrasound::FrameOutput frame;
    frame.timestamp_us = 1000U;
    frame.signal_ways.push_back({1000U, 1.0F, 0U, 1U});
    ultrasound::StaticFeature sf;
    sf.x_m = 1.0F;
    sf.y_m = 1.0F;
    sf.valid = true;
    frame.static_features.push_back(sf);

    ultrasound::DynamicFeature df;
    df.x_m = 1.0F;
    df.y_m = 1.0F;
    df.vx_mps = 0.1F;
    df.vy_mps = 0.0F;
    df.valid = true;
    frame.dynamic_features.push_back(df);

    ultrasound::LineMark lm;
    lm.x0_m = 0.0F;
    lm.y0_m = 0.0F;
    lm.x1_m = 1.0F;
    lm.y1_m = 0.0F;
    lm.valid = true;
    frame.line_marks.push_back(lm);
    frame.grid_map.valid = true;
    frame.processed.fused.push_back({1.0, 2.0});

    ultrasound::dispatch_runtime_frame(frame);

    EXPECT_TRUE(sw_called);
    EXPECT_TRUE(sf_called);
    EXPECT_TRUE(df_called);
    EXPECT_TRUE(lm_called);
    EXPECT_TRUE(gm_called);
    EXPECT_TRUE(pd_called);
}

TEST(RuntimeStubTest, ClearCallbacksStopsDispatch) {
    bool called = false;
    ultrasound::register_signal_ways_callback(
        [&](const std::vector<ultrasound::SignalWay>&, std::uint64_t) { called = true; });
    ultrasound::clear_runtime_callbacks();

    ultrasound::FrameOutput frame;
    frame.timestamp_us = 10U;
    frame.signal_ways.push_back({10U, 1.0F, 0U, 0U});
    ultrasound::dispatch_runtime_frame(frame);

    EXPECT_FALSE(called);
}

}  // namespace
