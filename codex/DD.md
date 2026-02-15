# DD (Detailed Design)

## Module Design

### `UltrasoundProcessor`
- Owns configuration, internal state queue, and algorithm units.
- Main API:
  - `initialize(config)`
  - `push_vehicle_state(state)`
  - `process_frame(frame, timestamp)`
  - `get_last_output()`

### `VehicleStateQueue`
- Ring buffer, monotonic insert.
- Interpolation method equivalent to `VehicleStateDataQueue` semantics.

### `HpcpToUssConverter`
- Converts `FusionFeatureMapper` fields:
  - mm->m conversion
  - std scaling by `nSigmaValeo`
  - probability scaling (`255` or legacy `100`)
  - height enum swap workaround for legacy defect mode
- Produces:
  - `UssStaticFeatures`
  - `UssSignalWays`

### `SignalWayPostProcessor`
- Reimplements/ports logic from legacy `UssSignalWaysInterface`:
  - detection via signal tracing
  - detection via FOV intersections
  - detection via ellipse intersections (sensor-pair ellipses + deterministic pairwise sampling)
  - cluster gating and fused measurement output
- configurable by:
  - group filter (front/rear/surround)
  - method selector (tracing/FOV/ellipse/all)

### `DiagnosticsStore`
- stage timing and counters
- exposed as immutable snapshot per output frame

### `ImGuiVisualizer`
- Standalone UI module fed by `std::vector<FrameOutput>`.
- Draws top-down overlays for tracing/FOV/ellipse/fused detections.
- Provides deterministic frame stepping, autoplay, scale controls.
- Draws vehicle contour and USS sensor rays from DAT vehicle INI (`[Contour]`, `[USS SENSORS]`).

### `VehicleGeometryLoader`
- Parses DAT vehicle INI geometry keys:
  - `contourPt*`
  - `uss_position_*`
  - `uss_mounting_*`
- Produces `VehicleGeometry` for visualizer and geometry-aware filtering.

## State Machines
Processing state per frame:
- `WAIT_INPUT` -> `DECODED` -> `STATE_RESOLVED` -> `CONVERTED` -> `POSTPROCESSED` -> `PUBLISHED`
- terminal error state at each stage with reason code

## Key Algorithms
1. Static feature conversion
- direct field mapping with unit normalization and validity checks.

2. Signal-way geometry
- sensor-pair mapping and geometric reconstruction.
- supports tracing/FOV/ellipse modes.
- front/rear signal-way IDs map to sensor pairs using legacy DAT order.
- ellipse method builds per-signal ellipses and computes deterministic intersection candidates.

3. Clustering/gating
- legacy-compatible distance gating threshold and grouping behavior.

4. Optional map/freespace integration
- adapter boundary retained; proprietary implementation replaceable.

## Configuration Schema
Sections (compatibility profile):
- `General` (mode flags, publish flags)
- `Conversion` (`nSigmaValeo`, legacy bugfix)
- `SignalWays` (`groupID`, `method`, thresholds)
- `GridMap` (width/height/cellSize and source controls)
- `Diagnostics` (verbosity, telemetry toggles)

## Thread Model
Default:
- single-thread deterministic processing pipeline.

Realtime extension:
- one ingest thread per transport adapter.
- one processing thread owning core state.
- lock-free or minimal-lock queue between ingest and processing.

Visualizer:
- single UI thread with deterministic playback index.
- no mutation of core processing state during rendering.

## Legacy-to-New Interface Mapping
- legacy callback entrypoints -> new `process_frame` and `push_vehicle_state`
- legacy packet IDs -> explicit output channel enum in new API
- legacy `IniFileParser` -> configuration provider interface with INI compatibility loader

## DD Status (2026-02-15)
- Processor, replay loader, config loader, runtime callback dispatch, and visualizer modules are implemented.
- Signal-way post-processing includes tracing, FOV-based, and ellipse-based methods with fused and clustered outputs.
- Vehicle geometry parsing for contour and USS sensors is implemented and wired to visualizer.
- Exact legacy parity in all geometry/render corner cases is still iterative and tracked as post-RC work.

