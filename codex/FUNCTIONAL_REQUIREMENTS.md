# Functional Requirements

FR-1 Ingest
- The processor shall ingest ultrasonic frames from replay and live transport adapters.

FR-2 Host State Association
- The processor shall associate each ultrasonic frame with an interpolated host state/pose at frame timestamp.

FR-3 Static Feature Output
- The processor shall convert ultrasonic map features to normalized static feature output with converted units and confidences.

FR-4 Signal-Way Output
- The processor shall convert ultrasonic signal-way inputs to normalized signal-way output including sensor pair metadata.

FR-5 Post-Processed Detections
- The processor shall provide post-processed detections generated from tracing, FOV intersection, and ellipse intersection methods.

FR-6 Clustered/Fused Output
- The processor shall provide clustered/fused pseudo-measurements from post-processed detections.

FR-7 Configurability
- The processor shall support runtime configuration of conversion and post-processing behavior via INI-compatible keys.

FR-8 Diagnostics
- The processor shall provide per-frame status and diagnostic counters.

FR-9 Replay Determinism
- Given identical replay input and config, the processor shall produce deterministic output sequences.

FR-10 Error Reporting
- The processor shall report per-frame errors and drops without crashing the processing loop.

FR-11 API Stability
- The processor shall expose a stable C++ API decoupled from legacy DAT framework classes.

FR-12 Integration Adapters
- The processor shall provide adapter interfaces for logging, time source, config loading, and transport publication.

FR-13 Visualization
- The module shall provide an ImGui-based visualizer to inspect frame-by-frame USS outputs in top-down view.

FR-14 Overlay Controls
- The visualizer shall allow toggling detection overlays (tracing, FOV intersections, ellipse intersections, fused).

FR-15 Playback Controls
- The visualizer shall support deterministic stepping and autoplay for replay outputs.

FR-16 Vehicle Geometry Rendering
- The visualizer shall render the host vehicle contour from DAT vehicle INI `[Contour]` points.
- The visualizer shall render USS sensor markers and mounting direction rays from `[USS SENSORS]`.

FR-17 Ellipse-Based Detection
- The processor shall compute ellipse-based detections using signal-way sensor-pair geometry and measured range.
- Intersections falling inside the host contour shall be rejected from ellipse detections.

## FR Implementation Status (2026-02-15)
- `FR-1` to `FR-17`: implemented for replay-first standalone scope.
- Live transport in `FR-1` and deep parity details in `FR-5/FR-17` are partial relative to full legacy runtime and remain planned enhancements.
- RC1 functional baseline is complete for deterministic replay processing + visualization.

