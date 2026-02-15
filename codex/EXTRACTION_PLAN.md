# Ultrasound Extraction Plan

## Objective
Create a standalone `Ultrasound-Processor` repository by extracting the ultrasound slice from `DAT_Resim_VS2015`, preserving behavior first, then modernizing build and integration boundaries.

## Extraction Basis
The extraction is based on concrete legacy wiring:
- Runtime host: `framework/DAT_realtime/Source/DAT_TrackerPC_console.cpp`
- USS ingest and conversion: `datasources/sensordrivers/ValeoSensorDriver/alg/src/UltrasonicSensorDriverInterface.cpp`
- USS dev/post-processing and map logic: `datasources/sensordrivers/ValeoSensorDriver/dev/src/ValeoSensorDriverDevInterface.cpp`
- Signal-way post-processing: `datasources/sensordrivers/ValeoSensorDriver/dev/src/UssSignalWaysInterface.cpp`
- Downstream consumers: `algorithms/MOGMAP2Algorithm/alg/src/MOGMAPInterface.cpp`, `algorithms/MOGMAP2Algorithm/MOGMAP2Dev/src/MOGMAP2Library.cpp`, `algorithms/Tracker/dev/src/ValeoSensorDriverCallback.cpp`
- Legacy target-selection path: `algorithms/PCAiAlgorithm/source/dev/PCAiLongRangeDownSelection.cpp`

## Scope Decision
- In scope for carve-out:
  - Valeo USS packet ingest/adaptation to normalized USS features and signal ways
  - Signal-way post-processing (tracing/FOV/ellipse, clustering)
  - Optional map/freespace adapter hooks
  - Deterministic replay path
  - Stable output API and telemetry
- Out of scope in first standalone drop:
  - Full DAT framework (`Library`, `InputBuffer`, `IntercomPacketEx`, Windows thread/event model)
  - Non-ultrasound radar/camera/LiDAR trackers and global runtime shell
  - Vendor-proprietary binaries (`uss_map_interface.lib`) in open build

## Phases
1. Inventory and boundary proof
2. Interface and architecture freeze
3. Carve-out implementation with adapters
4. Deterministic replay parity loop
5. Real-time integration path
6. Performance and quality gates

## Compatibility Strategy
- Preserve external data meaning and IDs where needed:
  - HPCP `FusionFeatureMapper_t` semantics
  - Intercom packet IDs used by downstream modules (1301-1306 family)
  - Existing INI key names where practical
- Preserve numeric behavior with tolerances:
  - Unit conversion, probability scaling, timestamp conventions
  - Signal-way post-processing outputs and clustering behavior

## Initial Success Criteria
- Standalone library builds with CMake+Conan
- Replay executable runs deterministically on fixed input
- Golden tests pass for selected legacy traces
- API docs and FR/NFR/HLD/DD are complete and reviewable

## Plan Status (2026-02-15)
- Phase 0/1/2/3 deliverables are completed for standalone replay-centric release.
- Phase 4 quality gates reached for unit tests and coverage threshold.
- Deferred items for next milestones:
  - full realtime transport parity
  - exact legacy solver/render behavior parity
  - CI/static-analysis hardening

