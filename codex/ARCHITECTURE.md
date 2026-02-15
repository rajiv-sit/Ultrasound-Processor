# Architecture

## System Context
Actors:
- Sensor source (replay file or live UDP Valeo USS)
- Host vehicle state source (localization)
- Ultrasound Processor (this module)
- Downstream consumers (tracker, mapping, parking logic, telemetry sink)

Inputs:
- Raw or decoded ultrasonic frame (`FusionFeatureMapper_t` semantics)
- Host state timeline and map-center updates
- Configuration profile (INI-compatible)

Outputs:
- Static feature list, signal-way list, post-processed pseudo-detections
- Optional occupancy/freespace map layers
- Diagnostic counters and timing telemetry

## Container View
1. `ultrasound_core`
- Pure algorithm and data model layer.
- No transport/threading assumptions.

2. `ultrasound_io`
- Adapters for replay input, UDP input, config source, time source, logging.
- Converts transport payloads to `ultrasound_core` input.

3. `ultrasound_apps`
- CLI replay runner and optional live bridge app.

4. `ultrasound_visualizer`
- ImGui/OpenGL visualization layer for replay and integration debugging.
- Isolated from core logic via `FrameOutput` API.

5. `ultrasound_tests`
- Unit and integration tests with golden vectors.

## Component View (core)
- `FrameDecoder`: raw payload -> normalized frame (optional, if encoded path used)
- `StateInterpolator`: host state interpolation per sensor timestamp
- `FeatureConverter`: HPCP map fields -> static features
- `SignalWayConverter`: HPCP signal-way fields -> signal-way data
- `SignalWayPostProcessor`:
  - tracing estimates
  - FOV intersection estimates
  - ellipse intersection estimates
  - clustering/fusion output
- `GridMapAdapter` (optional): map/freespace update hooks
- `Diagnostics`: timing, drop reasons, sequence counters

## Runtime View
Replay mode:
- deterministic single-thread loop
- fixed input order
- fixed state interpolation policy
- reproducible outputs for same input

Realtime mode:
- ingest thread(s) in IO adapter
- serialized processing in core execution queue
- monotonic timestamp guard to avoid out-of-order frame corruption

Visualizer mode:
- replay data -> core output frames -> ImGui top-down renderer
- selectable detection overlays (tracing/FOV/ellipse/fused)
- deterministic frame stepping for parity debugging

## Deployment View
Primary:
- Windows x64 (initial parity target)

Secondary:
- Linux x64 (when proprietary dependencies are stubbed or packaged)

Separation rule:
- platform-specific networking and OS APIs isolated in `ultrasound_io`
- `ultrasound_core` remains platform-agnostic C++

## Architecture Status (2026-02-15)
- Implemented containers: `ultrasound_core`, `ultrasound_io`, `ultrasound_visualizer`, apps.
- Runtime modes: replay path validated, realtime transport remains adapter-stubbed.
- Visualizer runtime path validated with contour, sensors, traces, FOV/ellipse/fused overlays.
- Architecture baseline is release-ready for public replay-centric usage.
- Open parity work remains for exact legacy rendering/solver equivalence.

