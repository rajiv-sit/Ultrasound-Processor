# Current Stage (2026-02-15)

## Overall Status
- Repository is standalone and IP-isolated from parent private repository code paths.
- Build system is Conan + CMake with VS2022 profile pinning for stability on machines that also have VS2026.
- Core processing + replay + runtime callback stubs + ImGui visualizer are implemented and buildable.
- Visualizer and processing parity workstreams are active; major USS visualization primitives are present.
- Release readiness assessment: `GO` for `v0.1.0-rc1` with documented limitations.

## Verified Results
- Unit tests: 20/20 passing (GoogleTest).
- Coverage gate: 82.81% line coverage over `src/core` + `src/io` (threshold: 80%).
- Non-visualizer build/test path verified.
- Visualizer build/run path verified by terminal execution.
- Formal sign-off checklist: `codex/RELEASE_READINESS.md`.

## Delivered Since Initial Carve-Out
- Removed parent-repo fallback dependencies from build scripts.
- Added replay typed records (`SW/SF/DF/LM/GM`) and loader support.
- Added runtime callback dispatch flow for signal ways, static/dynamic features, line marks, grid map, and processed detections.
- Added visualizer controls and overlays for:
  - signal-way traces
  - front-only filtering
  - FOV and ellipse intersection views
  - static/dynamic feature rendering
  - line marks and grid map rendering
  - sensor IDs and zoom
- Added sanitized public-facing data/config naming and reduced private/raw content exposure.

## Remaining / Partial Parity vs Legacy
- Exact parity of legacy ellipse traverse solver behavior remains approximation-based.
- Full parity of legacy render semantics for every draw path is still iterative.
- Full Valeo runtime transport integration remains stubbed (replay path is primary validated path).

## Standard Commands

### Build and test (no visualizer)
```powershell
$env:CONAN_HOME="D:\conan_cache"
conan install . -of build-test -pr:h=cmake/profiles/host-debug-vs2022 -pr:b=cmake/profiles/build-release-vs2022 -o "&:with_tests=True" -o "&:with_visualizer=False" --build=missing
cmake -S . -B build-test -DCMAKE_TOOLCHAIN_FILE="$PWD/build-test/build/generators/conan_toolchain.cmake" -DULTRASOUND_BUILD_TESTS=ON -DULTRASOUND_WITH_VISUALIZER=OFF -DULTRASOUND_ENABLE_COVERAGE=ON
cmake --build build-test --config Debug
ctest --test-dir build-test -C Debug --output-on-failure
cmake --build build-test --config Debug --target coverage
```

### Build and run visualizer
```powershell
$env:CONAN_HOME="D:\conan_cache"
conan install . -of build-viz -pr:h=cmake/profiles/host-debug-vs2022 -pr:b=cmake/profiles/build-release-vs2022 -o "&:with_visualizer=True" -o "&:with_tests=False" --build=missing
cmake -S . -B build-viz -DCMAKE_TOOLCHAIN_FILE="$PWD/build-viz/build/generators/conan_toolchain.cmake" -DULTRASOUND_WITH_VISUALIZER=ON -DULTRASOUND_BUILD_TESTS=OFF
cmake --build build-viz --config Debug
.\build-viz\Debug\uss_imgui_visualizer.exe .\replay\generated_from_legacy.csv .\configs\default_ultrasound_processor.ini .\configs\vehicle_profile_reference.ini
```

## Note on Conan Cache Permissions
If Conan reports readonly DB/cache errors, repair ACL once:
```powershell
icacls D:\conan_cache /grant "$env:USERNAME:(OI)(CI)F" /T /C
icacls D:\conan_cache\.conan.db /grant "$env:USERNAME:(F)"
```
