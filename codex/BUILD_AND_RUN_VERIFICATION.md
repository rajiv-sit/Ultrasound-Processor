# Build And Run Verification

## Status
- Core build/test path: `PASS`
- Legacy capture conversion path: `PASS`
- Replay processing path: `PASS`
- Conan visualizer build path: `PASS`
- Vehicle contour + USS sensor rendering path: `PASS`
- Ellipse detection method implementation path: `PASS`
- GoogleTest suite path: `PASS (20/20)`
- Coverage threshold path: `PASS (82.81% >= 80%)`

## Environment
- Workspace: `C:\Users\MrSit\source\repos\DAT_Resim_VS2015\Ultrasound-Processor`
- Conan home: `D:\conan_cache`
- Compiler/IDE generator for this repo: `Visual Studio 17 2022` (MSVC v143)
- Note: `SAR-processing` remains on VS2026; Ultrasound-Processor uses repo-local VS2022 Conan profiles.

## Commands Executed

### 1) Core build and tests (no visualizer)
```powershell
$env:CONAN_HOME="D:\conan_cache"
conan install . -of build-test -pr:h=cmake/profiles/host-debug-vs2022 -pr:b=cmake/profiles/build-release-vs2022 -o "&:with_tests=True" -o "&:with_visualizer=False" --build=missing
cmake -S . -B build-test -DCMAKE_TOOLCHAIN_FILE="$PWD/build-test/build/generators/conan_toolchain.cmake" -DULTRASOUND_BUILD_TESTS=ON -DULTRASOUND_WITH_VISUALIZER=OFF -DULTRASOUND_ENABLE_COVERAGE=ON
cmake --build build-test --config Debug
ctest --test-dir build-test -C Debug --output-on-failure
cmake --build build-test --config Debug --target coverage
```
Result:
- `100% tests passed, 0 tests failed out of 20`
- `line coverage: 82.81% (required: 80.00%)`

### 2) Convert legacy capture data -> replay CSV
```powershell
.\build-test\Debug\uss_legacy_capture_convert.exe .\data .\replay\generated_from_legacy.csv
```
Result:
- `Conversion completed: .\replay\generated_from_legacy.csv`

### 3) Replay runner on converted dataset
```powershell
.\build-test\Debug\uss_replay_runner.exe .\replay\generated_from_legacy.csv .\build-test\generated_output.csv .\configs\default_ultrasound_processor.ini
```
Observed output (expected behavior):
- `Dropped frame @0 reason=frame timestamp out of order`
- `processed=1472221 dropped=1`
- `filtered_signal_ways=1019723 clustered_detections=452498`
- runtime adapter remains stubbed by design

### 4) Conan install for visualizer (repo-local VS2022 profiles)
```powershell
conan install . -of build-viz `
  -pr:h=cmake/profiles/host-debug-vs2022 `
  -pr:b=cmake/profiles/build-release-vs2022 `
  -o "&:with_visualizer=True" `
  -o "&:with_tests=False" `
  --build=missing
```
Result:
- install completed successfully
- toolchain generated under:
  - `build-viz/build/generators/conan_toolchain.cmake`

### 5) Configure/build visualizer
```powershell
cmake -S . -B build-viz `
  -DCMAKE_TOOLCHAIN_FILE="$PWD/build-viz/build/generators/conan_toolchain.cmake" `
  -DULTRASOUND_WITH_VISUALIZER=ON `
  -DULTRASOUND_BUILD_TESTS=OFF
cmake --build build-viz --config Debug
```

### 6) Run visualizer with DAT vehicle geometry
```powershell
.\build-viz\Debug\uss_imgui_visualizer.exe `
  .\replay\generated_from_legacy.csv `
  .\configs\default_ultrasound_processor.ini `
  .\configs\vehicle_profile_reference.ini
```

### 7) Headless launch check for visualizer startup
```powershell
$p = Start-Process -FilePath .\build-viz\Debug\uss_imgui_visualizer.exe `
  -ArgumentList '.\replay\generated_from_legacy.csv','.\configs\default_ultrasound_processor.ini','.\configs\DAT_Vehicle_061W015_2021FEB22.ini' `
  -PassThru
Start-Sleep -Seconds 2
if ($p.HasExited) { "visualizer_exited=$($p.ExitCode)" } else { Stop-Process -Id $p.Id -Force; "visualizer_started_and_closed=ok" }
```
Result:
- `visualizer_started_and_closed=ok`

## Issues Encountered And Fixes

1. Conan generate conflict:
- Error: `CMakeToolchain is declared in generators attribute, but was instantiated in generate() method too`
- Fix: removed duplicate `generators = "CMakeDeps", "CMakeToolchain"` from `conanfile.py` and kept explicit `generate()`.

2. Wrong toolchain path in CMake call:
- Error: `Could not find toolchain file: build-viz/conan_toolchain`
- Fix: use cmake-layout path:
  - `build-viz/build/generators/conan_toolchain.cmake`

3. Visualizer header mismatch:
- Error: `Cannot open include file: 'imgui_impl_glfw.h'`
- Fix: switched includes in `src/visualization/imgui_visualizer.cpp` to:
  - `imgui_impl_glfw.hpp`
  - `imgui_impl_opengl3.hpp`

4. Added vehicle geometry extraction and rendering:
- Source sections:
  - `[Contour] contourPt*`
  - `[USS SENSORS] uss_position_*`
  - `[USS SENSORS] uss_mounting_*`
- New loader:
  - `load_vehicle_geometry_from_ini(...)`
- Visualizer now draws:
  - vehicle contour polyline
  - USS sensor points
  - sensor heading rays from mounting angles

5. Replaced placeholder ellipse method:
- Old behavior: scaled tracing placeholder (`x*1.05`, `y*1.05`)
- New behavior:
  - legacy-like signal-way to sensor-pair mapping
  - ellipse model creation from sensor baseline + measured range
  - deterministic pairwise ellipse intersection sampling
  - filtering of points falling inside host contour

## Artifacts Produced
- Generated replay CSV:
  - `replay/generated_from_legacy.csv`
- Generated replay output CSV:
  - `build-test/generated_output.csv`
- Binaries:
  - `build-test/Debug/uss_legacy_capture_convert.exe`
  - `build-test/Debug/uss_replay_runner.exe`
  - `build-viz/Debug/uss_imgui_visualizer.exe`

## Files Changed For This Flow
- `conanfile.py`
- `CMakeLists.txt`
- `src/visualization/imgui_visualizer.cpp`
- `apps/replay_runner.cpp`
- `apps/imgui_visualizer.cpp`
- `apps/legacy_capture_convert.cpp`
- `include/ultrasound/replay.hpp`
- `src/io/replay_source.cpp`
- `src/io/config_loader.cpp`
- `include/ultrasound/vehicle_geometry.hpp`
- `src/core/processor.cpp`
- `README.md`
- `cmake/profiles/host-debug-vs2022`
- `cmake/profiles/host-release-vs2022`
- `cmake/profiles/build-release-vs2022`

## Verification Update (2026-02-15)
- Latest validated results:
  - GoogleTest: `20/20` passed
  - Coverage: `82.81%` line coverage on `src/core` + `src/io`
  - Replay runner path: pass
  - Visualizer build/run path: pass
- Recommended canonical visualizer launch config file:
  - `configs/vehicle_profile_reference.ini`
- Historical notes in this file remain for traceability; `codex/CURRENT_STAGE.md` is the source of truth.
