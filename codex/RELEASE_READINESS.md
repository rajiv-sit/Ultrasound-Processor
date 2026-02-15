# Release Readiness

## Release Candidate
- Version tag target: `v0.1.0-rc1`
- Date: `2026-02-15`
- Scope: standalone replay-centric ultrasound processor + ImGui visualizer

## Go/No-Go Checklist
- [x] Repository is standalone and IP-isolated from parent private build paths.
- [x] Conan + CMake build flow documented and validated.
- [x] Replay processing apps build and execute.
- [x] Visualizer app builds and runs with sanitized in-repo config.
- [x] Unit tests are GoogleTest-based and passing.
- [x] Coverage gate is implemented and satisfied (`82.81% >= 80%`).
- [x] Public data policy documented (CSV/shareable only, raw private captures excluded).
- [x] Architecture/HLD/DD/FR/NFR docs updated to current implementation status.

## Known Limitations Accepted for RC1
- Full live transport parity with legacy runtime is not included.
- Exact legacy ellipse traverse parity is approximation-based.
- Exact draw parity for all legacy visualizer corner cases is still iterative.
- CI/static analysis mandatory gates are not yet enforced.

## Release Commands

### Test + coverage
```powershell
$env:CONAN_HOME="D:\conan_cache"
conan install . -of build-test -pr:h=cmake/profiles/host-debug-vs2022 -pr:b=cmake/profiles/build-release-vs2022 -o "&:with_tests=True" -o "&:with_visualizer=False" --build=missing
cmake -S . -B build-test -DCMAKE_TOOLCHAIN_FILE="$PWD/build-test/build/generators/conan_toolchain.cmake" -DULTRASOUND_BUILD_TESTS=ON -DULTRASOUND_WITH_VISUALIZER=OFF -DULTRASOUND_ENABLE_COVERAGE=ON
cmake --build build-test --config Debug
ctest --test-dir build-test -C Debug --output-on-failure
cmake --build build-test --config Debug --target coverage
```

### Visual verification
```powershell
$env:CONAN_HOME="D:\conan_cache"
conan install . -of build-viz -pr:h=cmake/profiles/host-debug-vs2022 -pr:b=cmake/profiles/build-release-vs2022 -o "&:with_visualizer=True" -o "&:with_tests=False" --build=missing
cmake -S . -B build-viz -DCMAKE_TOOLCHAIN_FILE="$PWD/build-viz/build/generators/conan_toolchain.cmake" -DULTRASOUND_WITH_VISUALIZER=ON -DULTRASOUND_BUILD_TESTS=OFF
cmake --build build-viz --config Debug
.\build-viz\Debug\uss_imgui_visualizer.exe .\replay\generated_from_legacy.csv .\configs\default_ultrasound_processor.ini .\configs\vehicle_profile_reference.ini
```

## Sign-Off
- Engineering recommendation: `GO` for RC1 public release with documented limitations.
