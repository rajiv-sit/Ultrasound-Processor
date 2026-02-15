# Ultrasound File Map

## Source -> Responsibility

- `framework/DAT_realtime/Source/DAT_TrackerPC_console.cpp`
  - Process bootstrap, thread orchestration, library loading from INI (`Lib0..LibN`), realtime mode shell.

- `framework/DAT_realtime/Source/ValeoUSSRx_Mailbox.cpp`
  - UDP multicast receive path for Valeo USS payloads (`224.100.100.100`), queueing and event signaling.

- `framework/DAT_realtime/Source/valeoUSSRx_ThreadFcn.cpp`
  - Latches decoded raw USS packets into library controller input for downstream callbacks.

- `framework/DAT_realtime/Source/ValeoUSSStreamInterface.h`
  - MUDP wrapper serialization for raw Valeo USS stream forwarding.

- `datasources/sensordrivers/ValeoSensorDriver/alg/include/UltrasonicSensorDriverInterface.h`
  - Production-like USS conversion interface and settings (`nSigmaValeo`, legacy bugfix flag).

- `datasources/sensordrivers/ValeoSensorDriver/alg/src/UltrasonicSensorDriverInterface.cpp`
  - Converts HPCP fusion feature mapper to normalized signal ways + static features.
  - Unit conversion mm->m, angle deg->rad, probability scaling, height enum bugfix handling.

- `datasources/sensordrivers/ValeoSensorDriver/dev/include/VSDDev/ValeoSensorDriverDevInterface.h`
  - Extended USS driver interface, INI settings, grid map/freespace integration, stream publication.

- `datasources/sensordrivers/ValeoSensorDriver/dev/src/ValeoSensorDriverDevInterface.cpp`
  - Main USS processing path.
  - Timestamp ordering control, map-center handling, dynamic features, line marks, grid maps.
  - Post-processing path for signal ways and optional UDP telemetry.
  - Registers and feeds Valeo ImGui visualization callbacks.

- `datasources/sensordrivers/ValeoSensorDriver/dev/include/VSDDev/ValeoSensorDriverVisualization.h`
  - USS visualization state contract, overlay toggles, and setter interfaces.

- `datasources/sensordrivers/ValeoSensorDriver/dev/src/ValeoSensorDriverVisualization.cpp`
  - ImGui panel and OpenGL drawing of gridmap, features, detections, and trace plots.

- `datasources/sensordrivers/ValeoSensorDriver/dev/include/VSDDev/UssSignalWaysInterface.h`
  - Post-processing API for signal-way pseudo-measurements and clustering.

- `datasources/sensordrivers/ValeoSensorDriver/dev/src/UssSignalWaysInterface.cpp`
  - Three detection modes: tracing, FOV intersections, ellipse intersections.
  - Contour exclusion and cluster gating/fusion.

- `datasources/sensordrivers/ValeoSensorDriver/dev/src/ValeoUltrasonicCallback.cpp`
  - Callback trigger on `UDP_VALEO_USS`; publishes localization, static, dynamic, signal-ways, gridmap packets.

- `datasources/sensordrivers/ValeoSensorDriver/dev/src/ValeoSensorDriverLibrary.cpp`
  - Library lifecycle and callback registration.
  - Integrates localization and map-center dependencies.

- `framework/DAT_framework/LibAPI/WindowsFrameworkIntercomPackets.h`
  - Defines packet IDs and packet wrapper types for Valeo USS outputs (1301-1306 range).

- `framework/DAT_framework/LibAPI/VisualizationBase.h/.cpp`
  - Shared ImGui/GLFW/OpenGL framework wrapper used by Valeo visualizer.

- `framework/DAT_framework/LibAPI/DatWorldVisualization.h/.cpp`
  - Docking host window and world visualization scheduler for all modules.

- `vendor/imgui/*`
  - Dear ImGui + GLFW/OpenGL backends used by DAT visualizers.

- `lib/DatCommon/AlgorithmCommon/include/DAT20InterAlgorithmTypes.h`
  - Defines `USSSignalWay_t`, `USSSignalWaysDataStruct_t`, `USSFeature_t`, `USSFeaturesDataStruct_t`.

- `lib/DatCommon/AlgorithmCommon/include/DAT20InterprocessorMessages.h`
  - Defines HPCP `FusionFeatureMapper_t` and ultrasonic counts/layout constraints.

- `algorithms/MOGMAP2Algorithm/MOGMAP2Dev/src/MOGMAP2Library.cpp`
  - Registers callbacks for Valeo signal ways/static features and forwards to dev interface.

- `algorithms/MOGMAP2Algorithm/MOGMAP2Dev/src/ValeoSignalWaysCallback.cpp`
  - Receives `VALEO_SIGNAL_WAYS_PACKET_ID` and calls `processUSSSignalWaysUpdate`.

- `algorithms/MOGMAP2Algorithm/MOGMAP2Dev/src/ValeoStaticFeaturesCallback.cpp`
  - Receives `VALEO_STATIC_FEATURES_PACKET_ID` and calls `processUSSStaticFeatureUpdate`.

- `algorithms/MOGMAP2Algorithm/alg/src/MOGMAPInterface.cpp`
  - Consumes USS static features and signal ways into obstacle manager/state machine.
  - Supports side-ULS gating queues and map/virtual-sensor downstream behavior.

- `algorithms/Tracker/dev/src/TrackerLibrary.cpp`
  - Registers Valeo packet event subscriptions for tracker.

- `algorithms/Tracker/dev/src/ValeoSensorDriverCallback.cpp`
  - Pulls static/dynamic/signal-way packets and dispatches tracker interface processing.

- `algorithms/PCAiAlgorithm/source/PCAiCallback.cpp`
  - Triggered on `UDP_U360_FUSION`; distinct target-selection path.

- `algorithms/PCAiAlgorithm/source/dev/PCAiLongRangeDownSelection.cpp`
  - Large legacy-generated target downselection logic (not core USS producer).

## Key Config Files
- `config/DAT_ValeoSensorDriver.ini` (USS driver behavior and post-processing)
- `config/DAT_MOGMAP2.ini` (USS consumption and state machine rates)
- `config/DAT_Tracker.ini` (USS callback enablement and stream timing)
- `config/DAT_Vehicle_*.ini` (`[USS SENSORS]` geometry/calibration, UDP network sections)
- `config/demo_resim_*.ini` and `config/DAT_TrackerPC_*.ini` (library chain selection)

## Filemap Status (2026-02-15)
- Legacy-to-standalone responsibility mapping is complete.
- Remaining gap items are parity refinements, not missing ownership mapping.

