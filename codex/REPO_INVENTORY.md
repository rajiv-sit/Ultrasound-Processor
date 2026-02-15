# Repository Inventory (Ultrasound Slice)

## High-Relevance Areas
- Runtime host and wiring:
  - `framework/DAT_realtime/Source/*`
- Ultrasound sensor driver:
  - `datasources/sensordrivers/ValeoSensorDriver/*`
- Ultrasound consumers:
  - `algorithms/MOGMAP2Algorithm/*`
  - `algorithms/Tracker/*`
- Legacy target-selection consumer path:
  - `algorithms/PCAiAlgorithm/*`
- Shared message/data model:
  - `framework/DAT_framework/LibAPI/WindowsFrameworkIntercomPackets.h`
  - `lib/DatCommon/AlgorithmCommon/include/DAT20InterAlgorithmTypes.h`
  - `lib/DatCommon/AlgorithmCommon/include/DAT20InterprocessorMessages.h`
- Configuration:
  - `config/DAT_ValeoSensorDriver.ini`
  - `config/DAT_MOGMAP2.ini`
  - `config/DAT_Tracker.ini`
  - `config/DAT_PCAiAlgorithm.ini`
  - `config/DAT_Vehicle_*.ini`
  - `config/demo_resim_*.ini`

## Approximate File Volume (from scoped listings)
- `algorithms/MOGMAP2Algorithm`: 322 files
- `datasources/sensordrivers/ValeoSensorDriver`: 39 files
- `algorithms/PCAiAlgorithm`: 27 files
- `algorithms/Tracker`: 134 files
- `framework/DAT_realtime/Source`: 89 files
- `config`: 133 files

## Runtime Mode Hooks Found
- Realtime host app and threads:
  - `framework/DAT_realtime/Source/DAT_TrackerPC_console.cpp`
  - `framework/DAT_realtime/Source/valeoUSSRx_ThreadFcn.cpp`
  - `framework/DAT_realtime/Source/ValeoUSSRx_Mailbox.cpp`
- Replay/resim execution via config library loading:
  - `config/demo_resim_*.ini`
  - `config/DAT_TrackerPC_*.ini`

## Core Algorithm and Glue Partition
Core-like logic:
- `datasources/sensordrivers/ValeoSensorDriver/alg/src/UltrasonicSensorDriverInterface.cpp`
- `datasources/sensordrivers/ValeoSensorDriver/dev/src/UssSignalWaysInterface.cpp`

Framework glue:
- `datasources/sensordrivers/ValeoSensorDriver/dev/src/ValeoSensorDriverLibrary.cpp`
- `datasources/sensordrivers/ValeoSensorDriver/dev/src/ValeoUltrasonicCallback.cpp`
- `framework/DAT_realtime/Source/*mailbox*`, `*ThreadFcn*`

Consumer integration:
- `algorithms/MOGMAP2Algorithm/MOGMAP2Dev/src/ValeoStaticFeaturesCallback.cpp`
- `algorithms/MOGMAP2Algorithm/MOGMAP2Dev/src/ValeoSignalWaysCallback.cpp`
- `algorithms/Tracker/dev/src/ValeoSensorDriverCallback.cpp`

## Inventory Status (2026-02-15)
- Legacy ultrasound slice inventory is complete and mapped to standalone modules.
- Inventory is sufficient for maintenance and future parity work without requiring private runtime coupling.

