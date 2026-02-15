# Ultrasound-Processor

`Ultrasound-Processor` is a standalone ultrasound processing and visualization repository focused on deterministic replay, algorithm validation, and incremental integration.

## Why This Repository Is Useful
- Provides a clean, public-facing ultrasound processing module with modern build tooling.
- Enables deterministic replay of ultrasound scenarios for debugging and regression testing.
- Includes an ImGui visualizer for top-down inspection of sensor geometry, signal ways, and detections.
- Separates algorithm core from IO/visualization for easier integration into larger systems.

## Scope
- `ultrasound_core`: frame processing, filtering, detection, fusion, clustering, diagnostics.
- `ultrasound_io`: replay loader, config loader, runtime callback stubs/adapters.
- `ultrasound_visualizer`: ImGui/OpenGL interactive visualization.
- Apps:
  - `uss_replay_runner`
  - `uss_legacy_capture_convert`
  - `uss_imgui_visualizer`

## IP / Isolation Boundary
- This repository is self-contained.
- Build scripts do not require source files outside this repository.
- Dependencies are resolved via Conan.
- Published inputs are CSV/config only; raw private captures are excluded.

## Prerequisites
- Windows x64.
- CMake `>= 3.30`.
- Conan (2.x command style).
- Visual Studio toolchain compatible with `Visual Studio 17 2022` generator.
- OpenGL-capable GPU/driver for visualizer.
- Optional for coverage: `OpenCppCoverage`.

## Quick Start

### 1) Install dependencies (tests only)
```powershell
$env:CONAN_HOME="D:\conan_cache"
conan install . -of build-test -pr:h=cmake/profiles/host-debug-vs2022 -pr:b=cmake/profiles/build-release-vs2022 -o "&:with_tests=True" -o "&:with_visualizer=False" --build=missing
```

### 2) Configure + build
```powershell
cmake -S . -B build-test -DCMAKE_TOOLCHAIN_FILE="$PWD/build-test/build/generators/conan_toolchain.cmake" -DULTRASOUND_BUILD_TESTS=ON -DULTRASOUND_WITH_VISUALIZER=OFF -DULTRASOUND_ENABLE_COVERAGE=ON
cmake --build build-test --config Debug
```

### 3) Run tests
```powershell
ctest --test-dir build-test -C Debug --output-on-failure
cmake --build build-test --config Debug --target coverage
```

## Build and Run (Visualizer)

### Configure dependencies for visualizer
```powershell
$env:CONAN_HOME="D:\conan_cache"
conan install . -of build-viz -pr:h=cmake/profiles/host-debug-vs2022 -pr:b=cmake/profiles/build-release-vs2022 -o "&:with_visualizer=True" -o "&:with_tests=False" --build=missing
```

### Configure + build visualizer
```powershell
cmake -S . -B build-viz -DCMAKE_TOOLCHAIN_FILE="$PWD/build-viz/build/generators/conan_toolchain.cmake" -DULTRASOUND_WITH_VISUALIZER=ON -DULTRASOUND_BUILD_TESTS=OFF
cmake --build build-viz --config Debug
```

### Run visualizer
```powershell
.\build-viz\Debug\uss_imgui_visualizer.exe .\replay\generated_from_legacy.csv .\configs\default_ultrasound_processor.ini .\configs\vehicle_profile_reference.ini
```

## Replay and Conversion Workflow

### Convert capture-like input to replay CSV
```powershell
.\build-test\Debug\uss_legacy_capture_convert.exe .\data .\replay\generated_from_legacy.csv
```

### Run replay processor
```powershell
.\build-test\Debug\uss_replay_runner.exe .\replay\generated_from_legacy.csv .\build-test\generated_output.csv .\configs\default_ultrasound_processor.ini
```

## Detection Methods (Implemented)

### 1) Signal Tracing
- Uses sensor mounting direction and measured range to estimate a detection point.
- For bistatic paths (different TX/RX sensors), combines directional vectors and projects to range.
- Useful as a direct, low-cost estimate.

### 2) FOV Intersection
- Monostatic (same TX/RX sensor): places point on the middle of the FOV arc at measured range.
- Bistatic (different TX/RX sensors): estimates source from intersection of sensor center rays and validates against sector/range constraints.
- Used as existence/plausibility support in fusion.

### 3) Ellipse Intersection
- Builds per-signal ellipses from sensor pair geometry and measured range.
- Computes intersection candidates between ellipse pairs (sampling + traverse-style approximation).
- Rejects points inside the vehicle contour.
- Provides higher geometric constraint than simple tracing.

## Fused Output Logic
- Candidate detections from tracing/FOV/ellipse are merged with uniqueness gating.
- Cross-method support is used when multiple methods are available.
- Final detections are clustered with distance-based table/melt logic.

## Visualizer Usage
- Playback:
  - frame stepping
  - autoplay
  - zoom in/out (mouse wheel)
- Overlays:
  - vehicle contour
  - USS sensor markers and IDs
  - signal-way traces
  - FOV-based detections
  - ellipse-based detections
  - fused and clustered detections
  - static features, dynamic features, line marks, grid map
- Coordinate convention in visualizer:
  - lateral/latitude mapped to X
  - longitudinal mapped to Y

## Figure / Video Placeholders

### Figure: Visualizer Main View
<img width="1913" height="1021" alt="image" src="https://github.com/user-attachments/assets/18ee7996-d4ba-4c5c-b26b-36f4f00aaf17" />

## Quality Status
- GoogleTest: `20/20` passing.
- Coverage gate: `82.81%` line coverage (`>=80%`) on `src/core` + `src/io`.
- Release track: `v0.1.0-rc1`.

## Repository Layout
- `include/ultrasound`: public API and data model
- `src/core`: processing algorithms
- `src/io`: replay/config/runtime adapters
- `src/visualization`: ImGui/OpenGL visualizer implementation
- `apps`: CLI/visualizer entrypoints
- `tests`: GoogleTest suites
- `bindings`: ImGui backend bridge sources
- `codex`: architecture, requirements, and release documentation

## Additional Documentation
- Current release stage and commands: `codex/CURRENT_STAGE.md`
- Release readiness/sign-off: `codex/RELEASE_READINESS.md`
- Architecture/HLD/DD/FR/NFR: `codex/`

## Further Info:
  https://github.com/rajiv-sit/Ultrasound-Processor
