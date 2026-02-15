# HLD

## Responsibilities
- Ingest ultrasonic and host-state data.
- Produce normalized USS outputs equivalent to legacy semantics.
- Provide deterministic replay and optional real-time transport integration.
- Expose stable API for downstream consumers.
- Provide an ImGui-based visual inspection tool for extracted USS outputs.
- Render host vehicle contour and USS sensor placements from DAT vehicle INI geometry.

## Dataflow (text diagram)
1. `InputAdapter` receives raw UDP or replay frame.
2. `Decoder` converts raw payload to `FusionFeatureMapper`-equivalent struct.
3. `StateInterpolator` computes observation pose for frame timestamp.
4. `FeatureConverter` and `SignalWayConverter` generate static/signal-way outputs.
5. `SignalWayPostProcessor` generates pseudo-detections and clustered outputs.
6. Optional `GridMapAdapter` computes occupancy/freespace hooks.
7. `OutputAdapter` publishes API objects/streams.
8. `Diagnostics` records latency/errors/counters.
9. `VisualizerAdapter` consumes frame outputs and renders top-down overlays.
10. `VehicleGeometryLoader` extracts `[Contour]` and `[USS SENSORS]` config for rendering and geometric checks.

## Error Handling
- Fail-loud on configuration schema errors at startup.
- Fail-soft per frame on malformed payloads or missing optional signals.
- Strict monotonic timestamp validation with drop counter.
- Structured error enum surfaced in API status.

## Diagnostics
- Per-stage timing (`decode`, `interpolate`, `convert`, `postprocess`, `publish`).
- Frame counters (`processed`, `dropped`, `late`, `decode_error`).
- Mode indicators (`replay`, `realtime`, `proprietary_map_enabled`).
- Visualizer state (`frame_index`, `autoplay`, overlay toggles).
- Geometry load status (`contour_points`, `sensor_count`, `vehicle_ini_path`).

## Performance Assumptions
- Frame processing budget target: <= 10 ms average, <= 20 ms p99 under nominal load.
- Target frame rate: compatible with legacy update cadence and replay throughput.
- Memory: bounded queues and fixed-size containers where possible.

## Determinism Notes
- In replay mode, all non-deterministic sources removed.
- Floating-point tolerance based parity checks for legacy matching.
- Stable sorting/iteration order enforced where clustering merges results.

## HLD Status (2026-02-15)
- Dataflow from replay ingestion through post-processing and rendering is implemented.
- Error handling and diagnostics behavior are active and test-covered.
- Performance instrumentation is present (stage timings and counters).
- Realtime live transport integration remains planned and not part of RC1 release scope.

