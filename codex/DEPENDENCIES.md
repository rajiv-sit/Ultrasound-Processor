# Dependencies

## External/3rd-Party (observed in ultrasound slice)
- `Eigen` (aligned allocators, matrix math)
- `Boost.Geometry` (polygon and point-in-polygon in signal-way processing)
- `Winsock2` / Windows sockets (UDP ingest)
- `Dear ImGui` (debug UI framework for world/USS visualization)
- `GLFW` (window/context and input handling for visualizer)
- `GLEW` (OpenGL function loading in legacy visualization stack)
- `OpenGL` (`opengl32` on Windows) for rendered overlays
- Valeo proprietary map library:
  - headers: `valeo/uss_map_interface_t_C.h`
  - link lib: `uss_map_interface.lib`

## Internal Shared Libraries and Headers
- `DAT_lib` framework abstractions (`Library`, callbacks, input/output buffer)
- `DatCommon` shared types and utilities
- `WindowsFrameworkIntercomPackets.h` packet IDs and wrappers
- `DAT20InterAlgorithmTypes.h` and `DAT20InterprocessorMessages.h`
- `VehicleStateDataQueue` and related state interpolation types

## Runtime/Integration Dependencies
- Localization provider (`LocalizationAlgorithm.dll`) for host pose interpolation and publication
- MOGMAP or MOGMAP2 map-center provider (optional source selection)
- Consumers (Tracker, MOGMAP2) reading Valeo packet IDs 1301-1306

## Build/Toolchain Constraints in Legacy
- Visual Studio toolset `v140` (VS2015)
- Windows SDK target `8.1`
- 32-bit Win32 dynamic libraries
- Runtime library flags `MultiThreaded` / `MultiThreadedDebug` (`/MT`, `/MTd`)
- Widespread Windows-specific APIs and threading model

## Porting Risks
- Proprietary Valeo binary availability on non-Windows targets
- ABI and packing assumptions in interop structures
- Implicit alignment requirements (`EIGEN_MAKE_ALIGNED_OPERATOR_NEW`)
- Legacy macro and enum behavior tied to DAT type definitions

## Proposed Conan Baseline for Standalone Repo
- Required (initial): none hard-required for core build beyond compiler/stdlib
- Optional:
  - `eigen/3.4.x`
  - `boost/1.86.x` (geometry only if retained)
  - `gtest/1.14.x` for tests
  - `glfw/3.4` + `glew/2.2.0` for ImGui visualizer build (`with_visualizer=True`)
- Proprietary bridge:
  - optional vendor package/channel for `uss_map_interface` or compile-time feature flag to stub

## Dependency Status (2026-02-15)
- Active Conan runtime dependencies:
  - `gtest/1.14.0` (tests)
  - `glfw/3.4`, `glew/2.2.0`, `imgui/cci.20230105+1.89.2.docking`, `opengl/system` (visualizer)
- Active tool dependency: `cmake/3.30.1`.
- Legacy DAT framework and proprietary Valeo binaries are not required for public standalone replay workflows.
- Repository is release-ready with Conan-managed open dependencies.

