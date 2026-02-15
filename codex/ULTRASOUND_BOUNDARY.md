# Ultrasound Boundary

## Module Name
`Ultrasound Processor`

## Public Contract
Inputs:
- Raw Valeo USS UDP payload (`DAT::ValeoUSSDataPacket_t`, 1261 bytes)
- Or decoded HPCP `DAT20InterprocessorMessages::FusionFeatureMapper_t`
- Vehicle state timeline (`DAT::VehicleStateData_t` equivalent)
- Optional map-center pose updates
- Configuration (INI-compatible)

Outputs:
- Normalized USS static features (`USSFeaturesDataStruct_t` equivalent)
- USS signal ways (`USSSignalWaysDataStruct_t` equivalent)
- Post-processed signal-way detections (`PostProcessedUSSSignalWaysDataStruct_t` equivalent)
- Optional grid/freespace layers
- Diagnostics/telemetry stream

Error model:
- Invalid timestamp order -> dropped frame + diagnostic counter
- Missing vehicle state at timestamp -> extrapolated/interpolated fallback + warning
- Malformed payload decode -> frame rejected + error code

## Internal Slice Included
- `datasources/sensordrivers/ValeoSensorDriver/alg/*`
- `datasources/sensordrivers/ValeoSensorDriver/dev/*` (ultrasound subset)
- Data types from `lib/DatCommon/AlgorithmCommon/include/DAT20InterAlgorithmTypes.h`
- HPCP type usage from `lib/DatCommon/AlgorithmCommon/include/DAT20InterprocessorMessages.h`

## Upstream Dependencies (Adapters)
- Time source abstraction (legacy: `DAT_TrackerPC::current_timestamp_us`)
- Config loader abstraction (legacy: `IniFileParser`)
- Input transport abstraction (legacy: UDP + mailbox thread)
- Logging abstraction (legacy: `DATLogger`)

## Downstream Integration Points
- MOGMAP2 callbacks consume:
  - `VALEO_SIGNAL_WAYS_PACKET_ID`
  - `VALEO_STATIC_FEATURES_PACKET_ID`
- Tracker callback consumes:
  - static, dynamic, signal ways
- PCAi remains optional and separate (not a core USS producer)

## Explicit Exclusions
- DAT full runtime shell, Windows event loop, CAN stack, non-USS sensor pipelines
- Full MOGMAP2, Tracker, and SPP algorithm stacks
- Non-ultrasound visualization framework

## Boundary Rationale
This keeps the ultrasound signal chain intact while decoupling framework glue. It enables incremental integration with deterministic replay first, then live transport.

## Boundary Status (2026-02-15)
- Public standalone boundary is implemented and enforced.
- Private DAT runtime dependencies are isolated behind adapters or excluded.
- Boundary is acceptable for public RC1 release.

